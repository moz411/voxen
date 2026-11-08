#!/usr/bin/env node
// Offline Open Brush .tilt -> VXMSH001 geometry compiler.
// Node.js >= 20; npm install --no-save three three-tiltloader fflate
import {readFile,writeFile} from 'node:fs/promises';
import {resolve} from 'node:path';
import {unzipSync,strFromU8} from 'fflate';
// Upstream source checkout is required: the npm tarball omits source modules.
// Clone https://github.com/icosa-foundation/three-tiltloader to tools/vendor/three-tiltloader.
import {readTiltStrokeData} from './vendor/three-tiltloader/src/tilt-strokes.js';
import {generateBrushGeometry} from './vendor/three-tiltloader/src/brush-geometry.ts';
import {getOpenBrushGeometryDefaults} from './vendor/three-tiltloader/src/brush-defaults.js';

const args=process.argv.slice(2);
const materialArg=args.indexOf('--materials');
let materialPath=null;
if(materialArg>=0){
 if(materialArg+1>=args.length)throw Error('--materials requires an output path');
 materialPath=args.splice(materialArg,2)[1];
}
const familyArg=args.indexOf('--family');
let onlyFamily=null;
if(familyArg>=0){
 if(familyArg+1>=args.length)throw Error('--family requires a value');
 onlyFamily=args.splice(familyArg,2)[1];
 if(!['ribbon','tube','hull','particle','emissive','thick-strip','concave-hull','print3d','unsupported'].includes(onlyFamily))
  throw Error('Unknown family: '+onlyFamily);
}
if(args.length!==3||args[1]!=='-o'){
 console.error('Usage: node tools/compile_openbrush.mjs scene.tilt -o scene.vxmesh [--family ribbon|tube|hull|particle] [--materials materials.json]');
 process.exit(2);
}
const source=await readFile(resolve(args[0]));
// Native .tilt has a 16-byte header before the ZIP payload.
if(source.length<20)throw Error('Truncated .tilt file');
const zip=unzipSync(source.subarray(16));
if(!zip['metadata.json']||!zip['data.sketch'])throw Error('Missing metadata.json or data.sketch');
const metadata=JSON.parse(strFromU8(zip['metadata.json']));
const strokes=readTiltStrokeData(zip['data.sketch'],metadata.BrushIndex);
const u32=v=>{const b=Buffer.allocUnsafe(4);b.writeUInt32LE(v);return b;};
const f32=a=>{const b=Buffer.allocUnsafe(a.length*4);for(let i=0;i<a.length;i++)b.writeFloatLE(a[i],i*4);return b;};
const uints=a=>{const b=Buffer.allocUnsafe(a.length*4);for(let i=0;i<a.length;i++)b.writeUInt32LE(a[i],i*4);return b;};
const chunks=[];let meshes=0,triangles=0,skipped=0;
const materialGroups=new Map();
const families=new Map();
for(const stroke of strokes){
 const guid=stroke.brushGuid.toLowerCase();
 const family=getOpenBrushGeometryDefaults(guid)?.family??'unsupported';
 families.set(family,(families.get(family)||0)+1);
 if(onlyFamily && family!==onlyFamily)continue;
 // Serialized positions and brush sizes are in decimeters, Unity-handed.
 const converted={
  ...stroke,brushSize:stroke.brushSize*.1,
  controlPoints:stroke.controlPoints.map(p=>({
   ...p,position:[p.position[0]*.1,p.position[1]*.1,-p.position[2]*.1],
   orientation:[-p.orientation[0],-p.orientation[1],p.orientation[2],p.orientation[3]]
  }))
 };
 const g=generateBrushGeometry(converted,family,{finalized:true,lastControlPointIsKeeper:true});
 const count=g.positions.length/3,ic=g.indices.length;
 if(!count||!ic){skipped++;continue;}
 const group=materialGroups.get(guid)??{guid,family,meshes:0,vertices:0,triangles:0,rendering:'unresolved',texture:null,blendMode:null};
 group.meshes++;group.vertices+=count;group.triangles+=ic/3;
 materialGroups.set(guid,group);
 if(!Number.isInteger(count)||count>10000000||ic>30000000)throw Error('Excessive mesh size');
 if(g.normals.length!==count*3||g.colors.length!==count*4)throw Error('Invalid attribute layout');
 // V1 stores UV0.xy; brush-specific packed UV0.zw, UV1, tangents and materials
 // are intentionally deferred to v2, not silently claimed to be faithful.
 const uv=g.packedUvs??g.uvs;
 const uvStride=g.uv0Size;
 const uv2=new Float32Array(count*2);
 for(let i=0;i<count;i++){uv2[i*2]=uv[i*uvStride];uv2[i*2+1]=uv[i*uvStride+1];}
 const bounds=[Infinity,Infinity,Infinity,-Infinity,-Infinity,-Infinity];
 for(let i=0;i<count;i++)for(let k=0;k<3;k++){
  const v=g.positions[i*3+k];
  if(!Number.isFinite(v))throw Error('Non-finite position');
  bounds[k]=Math.min(bounds[k],v);bounds[k+3]=Math.max(bounds[k+3],v);
 }
 for(const index of g.indices)if(index>=count)throw Error('Index outside vertex buffer');
 const guidBuf=Buffer.alloc(36);guidBuf.write(guid.slice(0,36),'ascii');
 chunks.push(guidBuf,u32(count),u32(ic),f32(bounds),f32(g.positions),f32(g.normals),f32(g.colors),f32(uv2),uints(g.indices));
 meshes++;triangles+=ic/3;
}
if(!meshes)throw Error('No geometry generated');
await writeFile(resolve(args[2]),Buffer.concat([Buffer.from('VXMSH001'),u32(1),u32(meshes),...chunks]));
if(materialPath){
 const manifest={schema:'voxen.openbrush.materials.v1',meshFormat:'VXMSH001',
  source:args[0],mesh:args[2],
  warning:'VXMSH001 retains GUID, UV0.xy and vertex colors, but not brush material semantics. Particle geometry is NOT equivalent to the original particle shader.',
  groups:[...materialGroups.values()].sort((a,b)=>b.meshes-a.meshes||a.guid.localeCompare(b.guid))};
 await writeFile(resolve(materialPath),JSON.stringify(manifest,null,2)+'\n');
}
console.log(JSON.stringify({strokes:strokes.length,meshes,triangles,skipped,filter:onlyFamily,families:Object.fromEntries(families),output:args[2]},null,2));

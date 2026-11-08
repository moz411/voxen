#!/usr/bin/env node
// Offline Open Brush -> Voxen mesh compiler (geometry only).
// Requires Node >=22 and npm install three three-tiltloader.
// VXMSH001: magic[8], version u32, meshCount u32, followed by per-mesh:
// guid[36] ASCII, vertexCount u32, indexCount u32, bounds[6] f32,
// positions[vertexCount*3] f32, normals[vertexCount*3] f32,
// colors[vertexCount*4] f32, uv[vertexCount*2] f32, indices[indexCount] u32.
// All numeric values little-endian; coordinates in meters, Y up, -Z forward.
import {readFile,writeFile} from 'node:fs/promises';
import {resolve} from 'node:path';
import {TiltLoader} from 'three-tiltloader';
const args=process.argv.slice(2);
if(args.length!==3 || args[1]!=='-o'){
 console.error('Usage: node tools/compile_openbrush.mjs scene.tilt -o scene.vxmesh');
 process.exit(2);
}
const bytes=await readFile(resolve(args[0]));
const loader=new TiltLoader();
const scene=await loader.parse(bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength));
const chunks=[];
const u32=v=>{const b=Buffer.alloc(4);b.writeUInt32LE(v);return b;};
const floats=a=>{const b=Buffer.alloc(a.length*4);a.forEach((v,i)=>b.writeFloatLE(v,i*4));return b;};
let meshCount=0,triangles=0;
scene.traverse(obj=>{
 if(!obj.isMesh)return;
 const g=obj.geometry;
 const position=g.getAttribute('position');
 if(!position || !g.index || !position.count || !g.index.count)return;
 const n=position.count,ic=g.index.count;
 const normal=g.getAttribute('normal'),color=g.getAttribute('color'),uv=g.getAttribute('uv');
 const data=(attr,width,defaultValues)=>{
  const a=new Float32Array(n*width);
  for(let i=0;i<n;i++)for(let j=0;j<width;j++)
   a[i*width+j]=attr&&j<attr.itemSize?attr.getComponent(i,j):defaultValues[j];
  return a;
 };
 const guid=String(obj.userData?.brushGuid||obj.material?.name||'unknown').slice(0,36);
 const guidBytes=Buffer.alloc(36);guidBytes.write(guid,'ascii');
 g.computeBoundingBox();
 const bb=g.boundingBox;
 const indices=new Uint32Array(ic);
 for(let i=0;i<ic;i++)indices[i]=g.index.getX(i);
 const chunksForMesh=[
  guidBytes,u32(n),u32(ic),
  floats([bb.min.x,bb.min.y,bb.min.z,bb.max.x,bb.max.y,bb.max.z]),
  floats(data(position,3,[0,0,0])),
  floats(data(normal,3,[0,1,0])),
  floats(data(color,4,[1,1,1,1])),
  floats(data(uv,2,[0,0])),
  Buffer.from(indices.buffer)
 ];
 chunks.push(...chunksForMesh);
 meshCount++;triangles+=ic/3;
});
if(!meshCount)throw Error('No supported geometry generated');
await writeFile(resolve(args[2]),Buffer.concat([Buffer.from('VXMSH001'),u32(1),u32(meshCount),...chunks]));
console.log(JSON.stringify({meshes:meshCount,triangles,output:args[2]}));

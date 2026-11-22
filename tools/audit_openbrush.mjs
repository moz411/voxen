#!/usr/bin/env node
// Inspect Open Brush .tilt files without generating geometry.
// Usage: node tools/audit_openbrush.mjs scene.tilt [-o report.json]
import {readFile,writeFile} from 'node:fs/promises';
import {resolve} from 'node:path';
import {unzipSync,strFromU8} from 'fflate';
import {readTiltStrokeData} from './vendor/three-tiltloader/src/tilt-strokes.js';
import {getOpenBrushGeometryDefaults} from './vendor/three-tiltloader/src/brush-defaults.js';

const args=process.argv.slice(2);
if(args.length!==1 && !(args.length===3 && args[1]==='-o')){
 console.error('Usage: node tools/audit_openbrush.mjs scene.tilt [-o report.json]');
 process.exit(2);
}
const source=await readFile(resolve(args[0]));
if(source.length<20)throw Error('Truncated .tilt file');
const zip=unzipSync(source.subarray(16));
if(!zip['metadata.json']||!zip['data.sketch'])throw Error('Missing metadata.json or data.sketch');
const metadata=JSON.parse(strFromU8(zip['metadata.json']));
const strokes=readTiltStrokeData(zip['data.sketch'],metadata.BrushIndex);
const brushes=new Map();
for(const stroke of strokes){
 const guid=stroke.brushGuid.toLowerCase();
 const defaults=getOpenBrushGeometryDefaults(guid);
 let item=brushes.get(guid);
 if(!item){
  item={guid,geometryFamily:defaults?.family??'unsupported',strokes:0,controlPoints:0,
   colorRange:{min:[Infinity,Infinity,Infinity,Infinity],max:[-Infinity,-Infinity,-Infinity,-Infinity]},
   sizeRange:{min:Infinity,max:-Infinity},
   material:{status:'unresolved',shader:null,blendMode:null,textureRefs:[]}};
  brushes.set(guid,item);
 }
 item.strokes++;
 item.controlPoints+=stroke.controlPoints.length;
 const color=stroke.color??[];
 for(let i=0;i<4;i++){
  const value=Number(color[i]??(i===3?1:0));
  if(!Number.isFinite(value))continue;
  item.colorRange.min[i]=Math.min(item.colorRange.min[i],value);
  item.colorRange.max[i]=Math.max(item.colorRange.max[i],value);
 }
 const size=Number(stroke.brushSize);
 if(Number.isFinite(size)){
  item.sizeRange.min=Math.min(item.sizeRange.min,size);
  item.sizeRange.max=Math.max(item.sizeRange.max,size);
 }
}
const entries=[...brushes.values()].sort((a,b)=>b.strokes-a.strokes||a.guid.localeCompare(b.guid));
for(const item of entries){
 if(item.sizeRange.min===Infinity)item.sizeRange={min:null,max:null};
 for(const k of ['min','max'])item.colorRange[k]=item.colorRange[k].map(v=>Number.isFinite(v)?v:null);
}
const report={
 schema:'voxen.openbrush.audit.v1',source:args[0],
 totals:{strokes:strokes.length,brushes:entries.length,controlPoints:entries.reduce((n,b)=>n+b.controlPoints,0)},
 note:'Geometry families are inferred from three-tiltloader defaults. Materials are deliberately unresolved until verified against Open Brush definitions.',
 brushes:entries
};
const output=JSON.stringify(report,null,2)+'\n';
if(args.length===3)await writeFile(resolve(args[2]),output);
else process.stdout.write(output);
console.error(`Audited ${strokes.length} strokes across ${entries.length} brush GUIDs`);

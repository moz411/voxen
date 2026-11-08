// Verified GUID identities from Open Brush-compatible brush catalogs.
// Rendering settings are intentionally not inferred from the geometry family.
export const knownBrushes = Object.freeze({
 '6a1cf9f9-032c-45ec-9b1d-a6680bee30f7': {
  name:'Dots', materialStatus:'identified', textureAsset:'Dots/MainTex',
  textureSource:'https://github.com/icosa-foundation/icosa-godot-addon/tree/main/addons/icosa/open_brush/brush_materials/Dots',
  blendMode:null,emission:null
 },
 '70d79cca-b159-4f35-990c-f02193947fe8': {
  name:'Smoke', materialStatus:'identified', textureAsset:'Smoke/MainTex',
  textureSource:'https://github.com/icosa-foundation/icosa-godot-addon/tree/main/addons/icosa/open_brush/brush_materials/Smoke',
  blendMode:null,emission:null
 }
});
export function identifyBrush(guid){
 return knownBrushes[guid.toLowerCase()]??{
  name:null,materialStatus:'unresolved',textureAsset:null,textureSource:null,
  blendMode:null,emission:null
 };
}

// Verified GUID identities from Open Brush-compatible brush catalogs.
// Rendering settings are intentionally not inferred from the geometry family.
export const knownBrushes = Object.freeze({
 '6a1cf9f9-032c-45ec-9b1d-a6680bee30f7': {
  name:'Dots', materialStatus:'identified', textureAsset:'Dots/MainTex',
  textureSource:'https://github.com/icosa-foundation/three-icosa/tree/main/brushes/Dots-6a1cf9f9-032c-45ec-9b1d-a6680bee30f7',
  referenceShader:'Dots-6a1cf9f9-032c-45ec-9b1d-a6680bee30f7-v10.0',
  particleAttributes:['normal_as_center','uv0_z_rotation','uv0_w_birth_time'],
  blendMode:null,emission:null
 },
 '70d79cca-b159-4f35-990c-f02193947fe8': {
  name:'Smoke', materialStatus:'identified', textureAsset:'Smoke/MainTex',
  textureSource:'https://github.com/icosa-foundation/three-icosa/tree/main/brushes/Smoke-70d79cca-b159-4f35-990c-f02193947fe8',
  referenceShader:'Smoke-70d79cca-b159-4f35-990c-f02193947fe8-v10.0',
  particleAttributes:['normal_as_center','uv0_z_rotation','uv0_w_birth_time'],
  animation:'curl_noise_world_space',
  blendMode:null,emission:null
 }
});
export function identifyBrush(guid){
 return knownBrushes[guid.toLowerCase()]??{
  name:null,materialStatus:'unresolved',textureAsset:null,textureSource:null,
  blendMode:null,emission:null
 };
}

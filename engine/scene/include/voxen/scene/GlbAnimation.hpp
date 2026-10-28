#pragma once
// CPU-side glTF TRS animation evaluation (node transformations only).
// Morph targets, skinned meshes, cubic spline tracks intentionally not yet supported.
#include "voxen/scene/GlbMeshDecoder.hpp"
#include <algorithm>
#include <cmath>
namespace voxen::scene::gltf {
using Matrix4=std::array<float,16>;
inline Matrix4 mul4(const Matrix4& a,const Matrix4& b){
 Matrix4 o{};
 for(int c=0;c<4;++c)for(int r=0;r<4;++r)
 for(int k=0;k<4;++k)o[c*4+r]+=a[k*4+r]*b[c*4+k];
 return o;
}
inline Matrix4 composeTRS(const std::array<float,3>& t,std::array<float,4> q,const std::array<float,3>& s){
 float n=std::sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
 if(n<1e-6f)q={0,0,0,1};else for(float& c:q)c/=n;
 float x=q[0],y=q[1],z=q[2],w=q[3];
 return { (1-2*(y*y+z*z))*s[0],(2*(x*y+z*w))*s[0],(2*(x*z-y*w))*s[0],0,
          (2*(x*y-z*w))*s[1],(1-2*(x*x+z*z))*s[1],(2*(y*z+x*w))*s[1],0,
          (2*(x*z+y*w))*s[2],(2*(y*z-x*w))*s[2],(1-2*(x*x+y*y))*s[2],0,
          t[0],t[1],t[2],1};
}
inline std::vector<Matrix4> sampleAnimation(const MeshAsset& asset,int clipIndex,float time){
 std::vector<Matrix4> world(asset.nodes.size());
 if(clipIndex<0 || static_cast<size_t>(clipIndex)>=asset.animations.size()) {
  for(size_t i=0;i<world.size();++i)world[i]=asset.nodes[i].worldMatrix;
  return world;
 }
 const auto& clip=asset.animations[static_cast<size_t>(clipIndex)];
 std::vector<std::array<float,3>> translations,scales;
 std::vector<std::array<float,4>> rotations;
 for(const auto& n:asset.nodes){translations.push_back(n.translation);scales.push_back(n.scale);rotations.push_back(n.rotation);}
 float tick=clip.duration>0.f?std::fmod(std::max(time,0.f),clip.duration):0.f;
 for(const auto& ch:clip.channels){
  if(ch.node<0||static_cast<size_t>(ch.node)>=asset.nodes.size()||ch.times.empty())continue;
  auto after=std::upper_bound(ch.times.begin(),ch.times.end(),tick);
  size_t a=after==ch.times.begin()?0:static_cast<size_t>(after-ch.times.begin()-1);
  size_t b=std::min(a+1,ch.times.size()-1);
  float delta=ch.times[b]-ch.times[a];
  float f=ch.step||delta<1e-6f?0.f:std::clamp((tick-ch.times[a])/delta,0.f,1.f);
  auto va=ch.values[a],vb=ch.values[b];
  if(ch.path==AnimationPath::Rotation){
   float dot=0.f;for(int k=0;k<4;++k)dot+=va[k]*vb[k];
   if(dot<0.f)for(float& v:vb)v=-v;
   std::array<float,4> q{};
   for(int k=0;k<4;++k)q[k]=va[k]+f*(vb[k]-va[k]);
   float len=0.f;for(float v:q)len+=v*v;
   len=std::sqrt(len);if(len>1e-6f)for(float& v:q)v/=len;
   rotations[ch.node]=q;
  }else{
   auto& target=ch.path==AnimationPath::Scale?scales[ch.node]:translations[ch.node];
   for(int k=0;k<3;++k)target[k]=va[k]+f*(vb[k]-va[k]);
  }
 }
 for(size_t i=0;i<world.size();++i){
  const auto& n=asset.nodes[i];
  auto local=n.hasMatrix?n.localMatrix:composeTRS(translations[i],rotations[i],scales[i]);
  world[i]=n.parent>=0&&static_cast<size_t>(n.parent)<i?mul4(world[n.parent],local):local;
 }
 return world;
}
} // namespace voxen::scene::gltf

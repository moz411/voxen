#include <voxen/audio/AmbientMusic.hpp>
#include <cmath>
#include <algorithm>

namespace voxen::audio {
bool AmbientMusic::start() noexcept {
#ifdef __ANDROID__
    if(stream_)return ready_.load();
    AAudioStreamBuilder* builder=nullptr;
    if(AAudio_createStreamBuilder(&builder)!=AAUDIO_OK)return false;
    AAudioStreamBuilder_setDirection(builder,AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setPerformanceMode(builder,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setSharingMode(builder,AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setFormat(builder,AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder,2);
    AAudioStreamBuilder_setDataCallback(builder,&AmbientMusic::callback,this);
    auto result=AAudioStreamBuilder_openStream(builder,&stream_);
    AAudioStreamBuilder_delete(builder);
    if(result!=AAUDIO_OK || !stream_)return false;
    sampleRate_=AAudioStream_getSampleRate(stream_);
    if(sampleRate_<=0)sampleRate_=48000;
    if(AAudioStream_requestStart(stream_)!=AAUDIO_OK){
        AAudioStream_close(stream_);stream_=nullptr;return false;
    }
    ready_.store(true);
    return true;
#else
    return false;
#endif
}
void AmbientMusic::stop() noexcept {
    ready_.store(false);
#ifdef __ANDROID__
    if(stream_){
        AAudioStream_requestStop(stream_);
        AAudioStream_close(stream_);
        stream_=nullptr;
    }
#endif
}
#ifdef __ANDROID__
aaudio_data_callback_result_t AmbientMusic::callback(AAudioStream*,void* userdata,void* buffer,int32_t numFrames){
    auto& self=*static_cast<AmbientMusic*>(userdata);
    auto* out=static_cast<float*>(buffer);
    const int track=self.track_.load(std::memory_order_relaxed);
    const float gain=self.playing_.load(std::memory_order_relaxed)?
        std::clamp(self.volume_.load(std::memory_order_relaxed),0.f,1.f)*0.24f:0.f;
    // Consume at most a few events/callback; no allocations or blocking.
    auto spawn=[&](int type,uint32_t count) {
        for(uint32_t j=0;j<std::min(count,6u);++j) {
            for(auto& e:self.effects_) {
                if(e.remaining>0)continue;
                e.type=type;
                const float seconds=type==0?0.095f:(type==1?0.16f:0.48f);
                e.total=e.remaining=std::max(1,int(seconds*self.sampleRate_));
                e.phase=0.f;
                break;
            }
        }
    };
    spawn(0,self.shots_.exchange(0,std::memory_order_relaxed));
    spawn(1,self.hits_.exchange(0,std::memory_order_relaxed));
    spawn(2,self.explosions_.exchange(0,std::memory_order_relaxed));
    constexpr double tau=6.283185307179586;
    const double step=1.0/std::max(1,self.sampleRate_);
    for(int32_t i=0;i<numFrames;i++){
        const double t=self.phase_;
        // Repeat seamlessly on a 120-second cycle. Harmonics are integral
        // multiples of the cycle frequency and therefore loop without clicks.
        const double cycle=tau*t/120.;
        self.seed_=1664525u*self.seed_+1013904223u;
        const float n=float((self.seed_>>8)&0xffffff)/8388608.f-1.f;
        self.filter_+=0.0018f*(n-self.filter_);
        float sound=0.f;
        if(track==0){ // COSMOS: layered slow, evolving drone
            const double mod=0.6+0.4*std::sin(cycle*2.);
            sound=float(0.55*std::sin(tau*55.*t)+
                0.25*std::sin(tau*82.5*t+0.5*std::sin(cycle*3.))+
                0.18*std::sin(tau*110.*t))*float(mod);
        } else if(track==1){ // OCEAN: synthetic long swell + gentle surf
            const float swell=float(0.48+0.42*std::sin(cycle*12.));
            self.wave_+=0.02f*(n-self.wave_);
            sound=0.65f*self.wave_*swell+
                0.6f*self.filter_+0.16f*std::sin(tau*98.*t)*swell;
        } else { // FOREST: soft air + distant pitched chirps
            const double chirp=std::pow(std::max(0.,std::sin(cycle*60.)),12.);
            sound=0.5f*self.filter_+
                float(0.3*chirp*std::sin(tau*(950.+150.*std::sin(cycle*60.))*t))+
                float(0.12*std::sin(tau*174.6*t));
        }
        float effect=0.f;
        for(auto& e:self.effects_) {
            if(e.remaining<=0)continue;
            const float progress=1.f-float(e.remaining)/float(e.total);
            const float duration=float(e.total)*float(step);
            const float envelope=(1.f-progress)*(1.f-progress);
            // Distinct procedural timbres: rising laser, metallic hit, noisy boom.
            const float freq=e.type==0?(640.f+930.f*progress):
                e.type==1?(440.f-270.f*progress):(145.f-112.f*progress);
            e.phase+=float(tau)*freq*float(step);
            if(e.phase>float(tau))e.phase-=float(tau);
            const float tonal=std::sin(e.phase);
            const float click=std::sin(e.phase*2.77f);
            const float value=e.type==0?(0.78f*tonal+0.22f*click):
                e.type==1?(0.46f*tonal+0.54f*n):
                           (0.40f*tonal+0.60f*n);
            effect+=value*envelope*(e.type==2?0.21f:0.13f);
            --e.remaining;
        }
        const float sample=std::clamp(sound*gain+effect,-0.85f,0.85f);
        out[2*i]=sample;
        out[2*i+1]=sample;
        self.phase_+=step;
        if(self.phase_>=120.)self.phase_-=120.;
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
#endif
} // namespace voxen::audio

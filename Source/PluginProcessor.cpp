#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioParameterFloat* pFloat(const juce::String& id, const juce::String& name, float lo, float hi, float def)
{ return new juce::AudioParameterFloat(id, name, juce::NormalisableRange<float>(lo, hi), def); }

RIVVocalForgeAudioProcessor::APVTS::ParameterLayout RIVVocalForgeAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout l;
    l.add(pFloat("input", "Input", -24, 24, 0)); l.add(pFloat("output", "Output", -24, 24, 0));
    l.add(pFloat("comp", "Compressor", 0, 100, 35)); l.add(pFloat("magic", "Magic", 0, 100, 25));
    l.add(pFloat("reverb", "Reverb", 0, 100, 12)); l.add(pFloat("delay", "Delay", 0, 100, 3));
    l.add(pFloat("air", "Air", 0, 12, 2)); l.add(pFloat("gate", "Gate", 0, 100, 15));
    l.add(pFloat("deess", "De-Esser", 0, 100, 25)); l.add(pFloat("threshold", "Comp Threshold", -36, 0, -18));
    l.add(pFloat("attack", "Attack", 1, 100, 18)); l.add(pFloat("release", "Release", 20, 500, 120));
    l.add(pFloat("ratio", "Ratio", 1, 8, 3)); l.add(pFloat("revtime", "Reverb Time", 0.2f, 8.0f, 1.8f));
    l.add(pFloat("delaytime", "Delay Time", 0.05f, 1.5f, 0.25f)); l.add(pFloat("delayhp", "Delay HP", 40, 1500, 180));
    l.add(pFloat("delaylp", "Delay LP", 1000, 18000, 7500));
    const float freqs[7] = { 80, 180, 400, 1000, 2500, 6000, 12000 };
    for (int i=0;i<7;++i) { l.add(pFloat("eqf"+juce::String(i+1), "EQ Frequency", 40, 18000, freqs[i])); l.add(pFloat("eqg"+juce::String(i+1), "EQ Gain", -12, 12, 0)); }
    return l;
}

RIVVocalForgeAudioProcessor::RIVVocalForgeAudioProcessor()
: AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  parameters(*this, nullptr, "RIV_STATE", createParameterLayout()) {}

bool RIVVocalForgeAudioProcessor::isBusesLayoutSupported(const BusesLayout& b) const
{ return b.getMainInputChannelSet() == b.getMainOutputChannelSet() && (b.getMainInputChannelSet() == juce::AudioChannelSet::mono() || b.getMainInputChannelSet() == juce::AudioChannelSet::stereo()); }

void RIVVocalForgeAudioProcessor::prepareToPlay(double sr, int samples)
{
    currentSampleRate = sr; maxAnalysisSamples = (int64_t)(sr * 60.0 * 15.0);
    delayBuffer.setSize(getTotalNumOutputChannels(), (int)(sr * 2.0) + samples + 1); delayBuffer.clear(); delayWrite = 0; envelope = 0;
    juce::dsp::Reverb::Parameters rp; rp.roomSize = 0.55f; rp.damping = 0.45f; rp.wetLevel = 0.25f; rp.dryLevel = 1.0f; rp.width = 1.0f; rp.freezeMode = 0.0f; reverb.setParameters(rp); reverb.setSampleRate(sr);
}
void RIVVocalForgeAudioProcessor::releaseResources() {}
float RIVVocalForgeAudioProcessor::value(const char* id) const { if (auto* p = parameters.getRawParameterValue(id)) return p->load(); return 0.0f; }

void RIVVocalForgeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels(), n = buffer.getNumSamples();
    const float inGain = juce::Decibels::decibelsToGain(value("input"));
    const float outGain = juce::Decibels::decibelsToGain(value("output"));
    const float comp = value("comp") / 100.0f, threshold = value("threshold"), ratio = value("ratio");
    const float gate = value("gate") / 100.0f, magic = value("magic") / 100.0f, air = value("air");
    const float deess = value("deess") / 100.0f, rev = value("reverb") / 100.0f, del = value("delay") / 100.0f;
    const float attack = std::exp(-1.0f / (float)(currentSampleRate * value("attack") * 0.001));
    const float release = std::exp(-1.0f / (float)(currentSampleRate * value("release") * 0.001));
    const int dSamples = juce::jlimit(1, delayBuffer.getNumSamples()-1, (int)(currentSampleRate * value("delaytime")));
    const float revTime = value("revtime");
    struct Biquad { float b0,b1,b2,a1,a2; } eq[7];
    for (int band=0; band<7; ++band) {
        const float f=juce::jlimit(40.0f,(float)currentSampleRate*0.45f,value(("eqf"+juce::String(band+1)).toRawUTF8()));
        const float g=value(("eqg"+juce::String(band+1)).toRawUTF8());
        const float A=std::pow(10.0f,g/40.0f), w=juce::MathConstants<float>::twoPi*f/(float)currentSampleRate;
        const float alpha=std::sin(w)/(2.0f*0.85f), c=std::cos(w), a0=1.0f+alpha/A;
        eq[band]={ (1.0f+alpha*A)/a0, (-2.0f*c)/a0, (1.0f-alpha*A)/a0, (-2.0f*c)/a0, (1.0f-alpha/A)/a0 };
    }
    auto rp = reverb.getParameters(); rp.roomSize = juce::jlimit(0.1f, 0.98f, 0.25f + revTime / 10.0f); rp.wetLevel = rev * 0.55f; rp.damping = 0.35f; reverb.setParameters(rp);
    double sumSq=0.0; float peak=0.0f, bright=0.0f; int64_t counted=0;
    for (int i=0;i<n;++i)
    {
        float mono=0.0f;
        for (int c=0;c<channels;++c) mono += buffer.getReadPointer(c)[i] / (float)channels;
        const float absIn = std::abs(mono); sumSq += mono*mono; peak = juce::jmax(peak, absIn);
        if (i>0) bright += std::abs(mono - (buffer.getReadPointer(0)[i-1])); ++counted;
    }
    if (analysing.load() && counted>0)
    {
        const auto old = analysisSamples.load(); const auto now = juce::jmin(maxAnalysisSamples, old + counted);
        const float blockRms = (float)std::sqrt(sumSq / counted);
        analysisRms.store(analysisRms.load()*0.92f + blockRms*0.08f); analysisPeak.store(juce::jmax(analysisPeak.load()*0.98f, peak));
        analysisBrightness.store(analysisBrightness.load()*0.92f + (bright/(float)counted)*0.08f);
        analysisSamples.store(now); analysisProgress.store((float)now/(float)maxAnalysisSamples);
        if (now >= maxAnalysisSamples) finishAnalysis();
    }
    for (int i=0;i<n;++i)
    {
        float mono=0.0f;
        for (int c=0;c<channels;++c) mono += buffer.getReadPointer(c)[i] / (float)channels;
        const float level = std::abs(mono); const float coeff = level > envelope ? attack : release; envelope = coeff*envelope + (1.0f-coeff)*level;
        const float db = juce::Decibels::gainToDecibels(envelope + 1.0e-8f);
        float gain = 1.0f;
        if (db > threshold) { const float compressedDb = threshold + (db-threshold)/juce::jmax(1.0f, ratio); gain *= juce::Decibels::decibelsToGain((compressedDb-db)*comp); }
        const float gateFloor = juce::jmap(gate, 0.0001f, 0.035f); if (envelope < gateFloor) gain *= juce::jmap(gate, 1.0f, 0.08f);
        for (int c=0;c<channels;++c)
        {
            float x = buffer.getReadPointer(c)[i] * inGain * gain;
            // Broad, restrained presence/air contour and harmonic saturation.
            x += (x - (i>0 ? buffer.getReadPointer(c)[i-1] : 0.0f)) * (air * 0.0018f + magic * 0.018f);
            x = std::tanh(x * (1.0f + magic*0.8f));
            for (int band=0; band<7; ++band) { const auto& q=eq[band]; const float y=q.b0*x+eqZ1[band][c]; eqZ1[band][c]=q.b1*x-q.a1*y+eqZ2[band][c]; eqZ2[band][c]=q.b2*x-q.a2*y; x=y; }
            // Gentle high-frequency de-essing approximation.
            if (deess > 0.001f && std::abs(x) > 0.35f) x *= (1.0f - deess*0.22f);
            const int read = (delayWrite - dSamples + delayBuffer.getNumSamples()) % delayBuffer.getNumSamples();
            const float delayed = delayBuffer.getSample(c, read);
            delayBuffer.setSample(c, delayWrite, x + delayed * del * 0.28f);
            x += delayed * del * 0.35f;
            buffer.setSample(c, i, x * outGain);
        }
        if (++delayWrite >= delayBuffer.getNumSamples()) delayWrite = 0;
    }
    if (rev > 0.001f) { if (channels > 1) reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), n); else reverb.processMono(buffer.getWritePointer(0), n); }
}

void RIVVocalForgeAudioProcessor::startVoiceAnalysis()
{ analysisRms=0; analysisPeak=0; analysisBrightness=0; analysisSamples=0; analysisProgress=0; analysisDone=false; analysing=true; }
void RIVVocalForgeAudioProcessor::stopVoiceAnalysis() { if (analysing.load()) finishAnalysis(); }
float RIVVocalForgeAudioProcessor::getAnalysisProgress() const { return analysisProgress.load(); }
juce::String RIVVocalForgeAudioProcessor::getAnalysisMessage() const { if (analysing) return "Analizando voz..."; if (analysisDone) return "Análisis aplicado. Ajustes adaptados a la señal."; return "Listo para analizar hasta 15 minutos."; }
void RIVVocalForgeAudioProcessor::finishAnalysis()
{
    if (!analysing.exchange(false)) return;
    const float rms=analysisRms.load(), peak=analysisPeak.load(), br=analysisBrightness.load();
    const float crest=peak / juce::jmax(0.0001f, rms);
    auto set=[this](const char* id,float v){ if(auto* p=parameters.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(v)); };
    set("input", juce::jlimit(-6.0f, 6.0f, -juce::Decibels::gainToDecibels(rms+1.0e-5f)-12.0f));
    set("comp", juce::jlimit(22.0f, 72.0f, 65.0f - crest*7.0f)); set("threshold", juce::jlimit(-30.0f,-10.0f, -22.0f + (rms<0.08f?3.0f:0.0f)));
    set("gate", rms<0.035f?24.0f:8.0f); set("deess", juce::jlimit(12.0f,55.0f, br*160.0f)); set("air", br<0.12f?4.0f:2.0f);
    set("magic", 24.0f); set("reverb", 10.0f); set("delay", 5.0f); analysisDone=true; analysisProgress=1.0f;
}

void RIVVocalForgeAudioProcessor::applyPreset(const juce::String& name)
{
    auto set=[this](const char* id,float v){ if(auto* p=parameters.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(v)); };
    float comp=36, magic=24, rev=10, del=4, air=2, de=24, gate=12, rt=1.7f, dt=.25f, th=-18, ratio=3;
    if(name.contains("Backing")){comp=52;magic=36;rev=48;del=22;rt=3.8f;dt=.5f;air=1;}
    else if(name.contains("Reggaeton")){comp=42;magic=30;rev=12;del=12;rt=1.6f;dt=.25f;air=3;}
    else if(name.contains("Big Reverb")){comp=40;rev=68;del=24;rt=6.5f;dt=.5f;}
    else if(name.contains("Trap")){comp=50;magic=38;rev=16;del=10;rt=1.3f;dt=.125f;th=-20;}
    else if(name.contains("Femenina")){comp=34;magic=26;rev=11;del=7;air=4;de=36;}
    if(name.contains("2")){magic+=5;rev+=3;del+=4;} if(name.contains("3")){comp+=7;magic-=3;} if(name.contains("4")){rev+=8;dt=.125f;} if(name.contains("5")){del+=8;dt=.375f;} if(name.contains("6")){air+=2;comp+=3;} if(name.contains("7")){magic+=9;rev+=5;}
    set("comp",comp);set("magic",magic);set("reverb",rev);set("delay",del);set("air",air);set("deess",de);set("gate",gate);set("revtime",rt);set("delaytime",dt);set("threshold",th);set("ratio",ratio);
}

void RIVVocalForgeAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{ auto s=parameters.copyState(); std::unique_ptr<juce::XmlElement> xml(s.createXml()); copyXmlToBinary(*xml,dest); }
void RIVVocalForgeAudioProcessor::setStateInformation(const void* data,int size)
{ std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,size)); if(xml && xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml)); }
juce::AudioProcessorEditor* RIVVocalForgeAudioProcessor::createEditor(){ return new RIVVocalForgeAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){ return new RIVVocalForgeAudioProcessor(); }

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>

class Modular404 final : public juce::AudioProcessor {
public:
    static constexpr int Count = 20;
    static constexpr const char* names[Count] = {"Vinyl Sim","Cassette","303 Vinyl","Lo-Fi","Bit Crusher","Isolator","Filter + Drive","Wah","DJFX Looper","Scatter","Chorus","Flanger","Phaser","Delay","Tape Echo","Reverb","Compressor","Saturator","Ring Mod","Tremolo"};
    juce::AudioProcessorValueTreeState state;
    Modular404() : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)), state(*this,nullptr,"PARAMS",makeLayout()) {}
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout() {
        juce::AudioProcessorValueTreeState::ParameterLayout p;
        for(int i=0;i<Count;++i) {
            auto k=juce::String(i);
            p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("on"+k,1),juce::String(names[i])+" Enabled",false));
            for(auto spec : {"mix","a","b","c"})
                p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(juce::String(spec)+k,1),juce::String(names[i])+" "+spec,juce::NormalisableRange<float>(0.f,1.f),spec==juce::String("mix")?0.5f:0.45f));
        }
        p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("bypass",1),"Global Bypass",false));
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("output",1),"Output",juce::NormalisableRange<float>(0.f,2.f),1.f));
        return p;
    }
    const juce::String getName() const override {return "404 Modular MKII";}
    bool acceptsMidi() const override {return false;} bool producesMidi() const override {return false;}
    double getTailLengthSeconds() const override {return 5.0;}
    bool isBusesLayoutSupported(const BusesLayout& b) const override {return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo() && b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
    int getNumPrograms() override {return 1;} int getCurrentProgram() override {return 0;}
    void setCurrentProgram(int) override {} const juce::String getProgramName(int) override {return {};}
    void changeProgramName(int,const juce::String&) override {}
    struct Cell {
        float lp=0, hp=0, previous=0, envelope=0, phase=0, hold=0, allpass=0, wow=0, second=0;
        int counter=0;
        size_t pos=0;
        std::vector<float> ring;
        void reset(size_t length) {lp=hp=previous=envelope=phase=hold=allpass=wow=second=0;counter=0;pos=0;ring.assign(length,0.f);}
        float tap(size_t offset) const {return ring[(pos+ring.size()-juce::jlimit(size_t(1),ring.size()-1,offset))%ring.size()];}
        float interpolatedTap(float offset) const {float clamped=juce::jlimit(1.f,float(ring.size()-2),offset);auto n=size_t(clamped);float f=clamped-float(n);return tap(n)*(1.f-f)+tap(n+1)*f;}
        void write(float v){ring[pos]=v;pos=(pos+1)%ring.size();}
    };
    std::array<std::array<Cell,2>,Count> cells;
    double sampleRate=44100;
    juce::Random random;
    void prepareToPlay(double sr,int) override {
        sampleRate=juce::jmax(8000.0,sr);
        auto length=size_t(sampleRate*2.1)+4;
        for(auto& effect:cells)for(auto& channel:effect)channel.reset(length);
    }
    void releaseResources() override {}
    static float clip(float x){return std::tanh(x);}
    float lowpass(Cell& s,float x,float hz) const {
        float k=1.f-std::exp(-juce::MathConstants<float>::twoPi*juce::jlimit(10.f,float(sampleRate)*0.45f,hz)/float(sampleRate));
        s.lp+=k*(x-s.lp);return s.lp;
    }
    float run(int fx,int ch,float x,float a,float b,float c) {
        auto& s=cells[size_t(fx)][size_t(ch)];
        const float fs=float(sampleRate), twopi=juce::MathConstants<float>::twoPi;
        s.phase+=1.f/fs; if(s.phase>10000.f)s.phase=0;
        float y=x;
        switch(fx) {
            case 0: { // vinyl: filtered bandwidth, noise and pitch modulation approximation
                float noise=(random.nextFloat()*2-1)*0.018f*b;
                float wobble=std::sin(twopi*s.phase*(0.4f+4*a))*0.008f*a;
                y=lowpass(s,x+noise+wobble,1500.f+14000.f*(1-c));break;
            }
            case 1: {float hiss=(random.nextFloat()*2-1)*0.013f*b; y=clip(lowpass(s,x+hiss,1500.f+13000.f*(1-c))*(1+4*a));break;}
            case 2: {float noise=(random.nextFloat()*2-1)*0.012f*b; y=clip(lowpass(s,x+noise,600.f+15000.f*(1-c))*(1+7*a));break;}
            case 3: {int steps=1+int(a*80);if(++s.counter>=steps){s.counter=0;s.hold=x;}float levels=std::pow(2.f,juce::jmap(b,4.f,16.f));y=std::round(s.hold*levels)/levels; y=lowpass(s,y,900.f+18000.f*(1-c));break;}
            case 4: {int steps=1+int(a*120);if(++s.counter>=steps){s.counter=0;s.hold=x;}float levels=std::pow(2.f,juce::jmap(b,2.f,16.f));y=std::round(s.hold*levels)/levels;break;}
            case 5: {float low=lowpass(s,x,80.f+450*a);float high=x-lowpassHigh(s,x,1600.f+8500*b);float mid=x-low-high;y=low*(0.3f+2*c)+mid+high*(0.3f+2*b);break;}
            case 6: {y=clip(lowpass(s,x,80.f+18000.f*std::pow(1-a,2.f))*(1+15*b));y*=1-0.6f*c;break;}
            case 7: {s.envelope+=(std::abs(x)-s.envelope)*0.005f;float f=180.f+7000.f*juce::jlimit(0.f,1.f,a+b*s.envelope);y=lowpass(s,x,f);y=clip(y*(1+4*c));break;}
            case 8: { // short looping / stutter, continuous circular recording
                size_t period=size_t(fs*(0.03f+0.6f*a));period=juce::jlimit(size_t(1),s.ring.size()-1,period);
                size_t local=s.pos%period;size_t offset=(b>0.65f ? (period-local) : period);
                y=s.tap(juce::jmax(size_t(1),offset));s.write(x); y=clip(y*(1+3*c));break;
            }
            case 9: { // rhythmic reverse-ish fragmented stutter approximation
                size_t period=juce::jlimit(size_t(1),s.ring.size()-1,size_t(fs*(0.035f+0.4f*a)));
                size_t step=(s.pos/period)%4;size_t offset=step%2?period-(s.pos%period):period/2;
                y=s.tap(juce::jmax(size_t(1),offset));s.write(x);y=clip(y*(1+2*b))*(1-0.5f*c);break;
            }
            case 10: {float mod=0.014f+0.009f*b*std::sin(twopi*s.phase*(0.1f+5*a));y=0.35f*x+0.65f*s.interpolatedTap(fs*mod);s.write(x);break;}
            case 11: {float mod=0.003f+0.0028f*b*std::sin(twopi*s.phase*(0.05f+6*a));float d=s.interpolatedTap(fs*mod);y=x+(0.4f+0.5f*c)*d;s.write(x+0.3f*d);break;}
            case 12: {float f=300.f+6500.f*(0.5f+0.5f*std::sin(twopi*s.phase*(0.08f+5*a)));float k=(1.f-std::tan(juce::MathConstants<float>::pi*f/fs))/(1.f+std::tan(juce::MathConstants<float>::pi*f/fs));k=juce::jlimit(-0.98f,0.98f,k);y=-k*x+s.previous+k*s.allpass;s.previous=x;s.allpass=y;y=x*(1-b)+y*b;break;}
            case 13: {size_t t=size_t(fs*(0.06f+1.25f*a));float echo=s.tap(t);y=echo;s.write(x+echo*(0.05f+0.87f*b));break;}
            case 14: {float wobble=1+0.01f*c*std::sin(twopi*s.phase*2.3f);size_t t=size_t(fs*(0.08f+0.9f*a)*wobble);float echo=s.interpolatedTap(fs*(0.08f+0.9f*a)*wobble);y=clip(echo*(1+2*c));s.write(x+echo*(0.1f+0.78f*b));break;}
            case 15: {size_t t=size_t(fs*(0.027f+0.13f*a));float echo=s.tap(t);float damp=lowpass(s,echo,1000.f+11000.f*(1-c));s.second+=0.22f*(damp-s.second);y=0.65f*echo+0.35f*s.second;s.write(x+s.second*(0.35f+0.58f*b));break;}
            case 16: {float e=std::abs(x);s.envelope+=(e-s.envelope)*(e>s.envelope?0.02f:0.0004f);float threshold=0.04f+0.55f*(1-a);float gain=s.envelope>threshold?std::pow(threshold/(s.envelope+1e-6f),0.3f+0.65f*b):1.f;y=x*gain*(1+2*c);break;}
            case 17: {y=clip(x*(1+35*a));y=lowpass(s,y,1000.f+18000.f*(1-b));y*=0.6f+0.8f*c;break;}
            case 18: {y=x*std::sin(twopi*s.phase*(5.f+3000.f*a));y=clip(y*(1+4*b));break;}
            case 19: {float mod=0.5f+0.5f*std::sin(twopi*s.phase*(0.2f+24*a));y=x*((1-b)+b*mod);y=clip(y*(1+3*c));break;}
        }
        return std::isfinite(y)?juce::jlimit(-8.f,8.f,y):0.f;
    }
    float lowpassHigh(Cell& s,float x,float hz) const {float k=1.f-std::exp(-juce::MathConstants<float>::twoPi*hz/float(sampleRate));s.hp+=k*(x-s.hp);return s.hp;}
    void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override {
        juce::ScopedNoDenormals guard;
        std::array<bool,Count> on{};
        std::array<std::array<float,4>,Count> v{};
        for(int i=0;i<Count;++i){auto k=juce::String(i);on[size_t(i)]=state.getRawParameterValue("on"+k)->load()>0.5f;
            for(int j=0;j<4;++j)v[size_t(i)][size_t(j)]=state.getRawParameterValue(juce::String(std::array<const char*,4>{"mix","a","b","c"}[size_t(j)])+k)->load();}
        if(state.getRawParameterValue("bypass")->load()>0.5f)return;
        float gain=state.getRawParameterValue("output")->load();
        for(int n=0;n<buffer.getNumSamples();++n)for(int ch=0;ch<juce::jmin(2,buffer.getNumChannels());++ch){
            float x=buffer.getSample(ch,n);
            for(int i=0;i<Count;++i)if(on[size_t(i)]){
                auto& q=v[size_t(i)];float wet=run(i,ch,x,q[1],q[2],q[3]);x=x*(1-q[0])+wet*q[0];
            }
            buffer.setSample(ch,n,juce::jlimit(-4.f,4.f,x*gain));
        }
    }
    bool hasEditor() const override {return true;}
    juce::AudioProcessorEditor* createEditor() override;
    void getStateInformation(juce::MemoryBlock& dest) override {auto xml=state.copyState().createXml();copyXmlToBinary(*xml,dest);}
    void setStateInformation(const void* data,int size) override {auto xml=getXmlFromBinary(data,size);if(xml&&xml->hasTagName(state.state.getType()))state.replaceState(juce::ValueTree::fromXml(*xml));}
};

class ModularEditor final : public juce::AudioProcessorEditor {
public:
    ModularEditor(Modular404& p):AudioProcessorEditor(p),processor(p){
        setSize(1080,790);
        for(int i=0;i<Modular404::Count;++i){auto& m=modules[size_t(i)];
            m.title.setText(Modular404::names[i],juce::dontSendNotification);m.title.setJustificationType(juce::Justification::centredLeft);m.title.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(m.title);
            m.on.setButtonText("ON");addAndMakeVisible(m.on);
            m.onAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"on"+juce::String(i),m.on);
            for(int j=0;j<4;++j){auto& slider=m.knobs[size_t(j)];slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,52,17);slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffed9c54));addAndMakeVisible(slider);
                m.attach[size_t(j)]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,juce::String(std::array<const char*,4>{"mix","a","b","c"}[size_t(j)])+juce::String(i),slider);
            }
        }
        master.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);master.setTextBoxStyle(juce::Slider::TextBoxBelow,false,65,20);addAndMakeVisible(master);
        masterAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"output",master);
        bypass.setButtonText("BYPASS");addAndMakeVisible(bypass);bypassAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"bypass",bypass);
        preset.addItem("Choose starting preset...",1);preset.addItem("Vinyl Memories",2);preset.addItem("Dusty Boom Bap",3);preset.addItem("Tape Haze",4);preset.addItem("Space Echo",5);preset.addItem("Broken Digital",6);preset.onChange=[this]{loadPreset(preset.getSelectedId());};addAndMakeVisible(preset);
        scroll.setRangeLimits(0,3);scroll.setCurrentRange(0,1);scroll.onValueChange=[this]{resized();repaint();};addAndMakeVisible(scroll);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff11151c));g.setColour(juce::Colours::white);g.setFont(juce::Font(juce::FontOptions(28.f,juce::Font::bold)));
        g.drawText("404 MODULAR  /  MKII",24,12,760,45,juce::Justification::left);
        g.setFont(juce::Font(juce::FontOptions(13.f)));g.setColour(juce::Colour(0xffadb5c3));g.drawText("20 independent FX  |  MIX / CHARACTER / MOTION / TONE  |  original DSP",25,55,800,22,juce::Justification::left);
        int top=scrollRow();
        for(int i=top*4;i<juce::jmin(Modular404::Count,(top+3)*4);++i){auto r=tile(i);g.setColour(juce::Colour(0xff242c37));g.fillRoundedRectangle(r.toFloat(),12);g.setColour(juce::Colour(0xff465369));g.drawRoundedRectangle(r.toFloat(),12,1);
            g.setColour(juce::Colour(0xffb6bfcb));const char* labels[]={"MIX","CHAR","MOVE","TONE"};for(int j=0;j<4;++j)g.drawText(labels[j],r.getX()+5+j*61,r.getY()+128,60,16,juce::Justification::centred);
        }
        g.setColour(juce::Colour(0xffaab4c5));g.drawText("MASTER",840,27,95,22,juce::Justification::centred);
        g.drawText("Independent sound design – not affiliated with Roland",25,757,650,20,juce::Justification::left);
    }
    int scrollRow() const {return juce::jlimit(0,2,int(scroll.getCurrentRangeStart()+0.5));}
    juce::Rectangle<int> tile(int i) const {int col=i%4,row=i/4-scrollRow();return {20+col*260,100+row*212,248,194};}
    void resized() override {
        for(int i=0;i<Modular404::Count;++i){auto r=tile(i);auto& m=modules[size_t(i)];bool visible=(i/4>=scrollRow()&&i/4<scrollRow()+3);
            m.title.setVisible(visible);m.on.setVisible(visible);m.title.setBounds(r.getX()+12,r.getY()+10,160,25);m.on.setBounds(r.getRight()-65,r.getY()+10,60,25);
            for(int j=0;j<4;++j){m.knobs[size_t(j)].setVisible(visible);m.knobs[size_t(j)].setBounds(r.getX()+4+j*61,r.getY()+49,60,87);}
        }
        master.setBounds(928,14,115,78);bypass.setBounds(780,22,105,26);preset.setBounds(650,58,225,26);scroll.setBounds(1050,100,16,628);
    }
private:
    void loadPreset(int selected) {
        if(selected<2)return;
        // Preset changes are undoable and all parameters remain host-automatable.
        auto set=[this](const juce::String& id,float value){if(auto* param=processor.state.getParameter(id))param->setValueNotifyingHost(param->convertTo0to1(value));};
        for(int i=0;i<Modular404::Count;++i){auto k=juce::String(i);set("on"+k,0.f);set("mix"+k,0.5f);set("a"+k,0.45f);set("b"+k,0.45f);set("c"+k,0.45f);}
        auto fx=[&](int i,float mix,float a,float b,float c){auto k=juce::String(i);set("on"+k,1.f);set("mix"+k,mix);set("a"+k,a);set("b"+k,b);set("c"+k,c);};
        if(selected==2){fx(0,.75f,.35f,.5f,.55f);fx(5,.25f,.3f,.45f,.6f);}
        if(selected==3){fx(0,.5f,.35f,.4f,.65f);fx(3,.55f,.3f,.5f,.6f);fx(16,.4f,.55f,.65f,.4f);}
        if(selected==4){fx(1,.8f,.5f,.65f,.65f);fx(10,.3f,.15f,.6f,.4f);fx(17,.2f,.25f,.2f,.55f);}
        if(selected==5){fx(14,.65f,.4f,.7f,.5f);fx(15,.55f,.45f,.65f,.65f);fx(12,.25f,.3f,.65f,.5f);}
        if(selected==6){fx(4,.75f,.75f,.25f,.5f);fx(9,.45f,.2f,.7f,.5f);fx(18,.25f,.5f,.4f,.5f);}
    }
    struct Module {juce::Label title;juce::ToggleButton on;std::array<juce::Slider,4> knobs;std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttachment;std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,4> attach;};
    Modular404& processor;std::array<Module,Modular404::Count> modules;juce::Slider master;juce::ToggleButton bypass;juce::ComboBox preset;std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;juce::ScrollBar scroll{true};std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttach;
};
juce::AudioProcessorEditor* Modular404::createEditor(){return new ModularEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Modular404();}

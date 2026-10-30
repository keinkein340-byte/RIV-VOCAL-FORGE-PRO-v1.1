#include "PluginEditor.h"

static juce::Colour bg(0xff101518), panel(0xff192126), orange(0xffff9700), text(0xffedf1f2), muted(0xffaab4b8);

RIVVocalForgeAudioProcessorEditor::RIVVocalForgeAudioProcessorEditor(RIVVocalForgeAudioProcessor& p)
: AudioProcessorEditor(&p), processor(p), progress(progressValue)
{
    setSize(1060, 720); setResizable(true,true); setResizeLimits(820,560,1500,1000);
    brand.setText("RIV VOCAL FORGE", juce::dontSendNotification); brand.setFont(juce::Font(25.0f,juce::Font::bold)); brand.setColour(juce::Label::textColourId,text); addAndMakeVisible(brand);
    for (auto s : {"MID CONTROL 1","MID CONTROL 2","MID CONTROL 3","BACKING VOCAL 1","BACKING VOCAL 2","REGGAETON 1","REGGAETON 2","REGGAETON 3","REGGAETON 4","REGGAETON 5","REGGAETON 6","REGGAETON 7","REGGAETON BIG REVERB","TRAP 1","TRAP 2","VOZ FEMENINA 1","VOZ FEMENINA 2"}) presets.addItem(s,presets.getNumItems()+1);
    presets.setSelectedId(3); addAndMakeVisible(presets);
    loadButton.onClick=[this]{ if(presets.getSelectedId()>0) processor.applyPreset(presets.getText()); }; addAndMakeVisible(loadButton);
    saveButton.onClick=[this]{ juce::FileChooser chooser("Guardar preset RIV",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("RIV Vocal Presets"),"*.rivpreset"); if(chooser.browseForFileToSave(true)){ auto file=chooser.getResult().withFileExtension("rivpreset"); juce::MemoryBlock b; processor.getStateInformation(b); file.replaceWithData(b.getData(),b.getSize()); } }; addAndMakeVisible(saveButton);
    importButton.onClick=[this]{ juce::FileChooser chooser("Cargar preset RIV",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.rivpreset"); if(chooser.browseForFileToOpen()){ juce::MemoryBlock b; if(chooser.getResult().loadFileAsData(b)) processor.setStateInformation(b.getData(),(int)b.getSize()); } }; addAndMakeVisible(importButton);
    aiButton.onClick=[this]{ if(processor.isAnalysing()) processor.stopVoiceAnalysis(); else processor.startVoiceAnalysis(); }; addAndMakeVisible(aiButton);
    aiStatus.setColour(juce::Label::textColourId,muted); addAndMakeVisible(aiStatus); addAndMakeVisible(progress);
    tabs.addTab("SIMPLE",orange,&simplePage,false); tabs.addTab("AVANZADO",orange,&advancedPage,false); addAndMakeVisible(tabs);
    addControl(simplePage,"comp","COMPRESOR",{}); addControl(simplePage,"magic","MAGIC",{}); addControl(simplePage,"reverb","REVERB",{}); addControl(simplePage,"delay","DELAY",{});
    addControl(advancedPage,"input","INPUT",{}); addControl(advancedPage,"output","OUTPUT",{}); addControl(advancedPage,"comp","COMPRESOR",{}); addControl(advancedPage,"threshold","THRESHOLD",{}); addControl(advancedPage,"ratio","RATIO",{}); addControl(advancedPage,"attack","ATTACK",{}); addControl(advancedPage,"release","RELEASE",{});
    addControl(advancedPage,"air","AIR",{}); addControl(advancedPage,"gate","NOISE GATE",{}); addControl(advancedPage,"deess","DE-ESSER",{}); addControl(advancedPage,"revtime","REVERB TIME",{}); addControl(advancedPage,"delaytime","DELAY TIME",{}); addControl(advancedPage,"delayhp","DELAY HP",{}); addControl(advancedPage,"delaylp","DELAY LP",{});
    for(int i=1;i<=7;++i){addControl(advancedPage,"eqf"+juce::String(i),"EQ FREQ "+juce::String(i),{});addControl(advancedPage,"eqg"+juce::String(i),"EQ GAIN "+juce::String(i),{});}
    startTimerHz(8);
}

void RIVVocalForgeAudioProcessorEditor::addControl(juce::Component& parent,const juce::String& id,const juce::String& label,juce::Rectangle<int>,bool rotary)
{
    auto s=std::make_unique<Slider>(); s->setName(label); s->setSliderStyle(rotary?Slider::RotaryHorizontalVerticalDrag:Slider::LinearHorizontal); s->setTextBoxStyle(Slider::TextBoxBelow,false,74,22); s->setColour(Slider::rotarySliderFillColourId,orange); s->setColour(Slider::thumbColourId,orange); s->setColour(Slider::textBoxTextColourId,text); s->setColour(Slider::textBoxOutlineColourId,juce::Colour(0xff333d42));
    auto labelPtr=new juce::Label(); labelPtr->setText(label,juce::dontSendNotification); labelPtr->setJustificationType(juce::Justification::centred); labelPtr->setColour(juce::Label::textColourId,muted); labelPtr->setName("label_"+id); labels.add(labelPtr); parent.addAndMakeVisible(labelPtr);
    parent.addAndMakeVisible(*s); attachments.push_back(std::make_unique<RIVVocalForgeAudioProcessor::APVTS::SliderAttachment>(processor.parameters,id,*s)); sliders.push_back(std::move(s));
}

void RIVVocalForgeAudioProcessorEditor::timerCallback()
{
    progressValue=processor.getAnalysisProgress(); aiStatus.setText(processor.getAnalysisMessage(),juce::dontSendNotification);
    aiButton.setButtonText(processor.isAnalysing()?"DETENER Y APLICAR":"ANALIZAR VOZ · 15 MIN");
}

void RIVVocalForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg); g.setColour(orange); g.drawRoundedRectangle(8,8,(float)getWidth()-16,(float)getHeight()-16,12,1.2f);
}

void RIVVocalForgeAudioProcessorEditor::resized()
{
    auto r=getLocalBounds().reduced(22); auto top=r.removeFromTop(48); brand.setBounds(top.removeFromLeft(330)); presets.setBounds(top.removeFromLeft(260).reduced(4)); loadButton.setBounds(top.removeFromLeft(125).reduced(4)); importButton.setBounds(top.removeFromLeft(120).reduced(4)); saveButton.setBounds(top.removeFromLeft(125).reduced(4));
    auto ai=r.removeFromTop(70); aiButton.setBounds(ai.removeFromLeft(245).reduced(5)); aiStatus.setBounds(ai.removeFromLeft(360).reduced(5)); progress.setBounds(ai.reduced(8)); tabs.setBounds(r);
    // Lay out the Simple page as four large macro knobs.
    auto sr=simplePage.getLocalBounds().reduced(24); const int w=sr.getWidth()/4;
    for(size_t i=0;i<4 && i<sliders.size();++i){ auto* s=sliders[i].get(); s->setBounds(sr.getX()+(int)i*w+8,sr.getY()+45,w-16,sr.getHeight()-80); }
    // Advanced page: compact 5-column control grid. Remaining EQ bands are scroll-free and accessible by resizing.
    auto ar=advancedPage.getLocalBounds().reduced(10); const int cols=5, cellW=ar.getWidth()/cols, cellH=118;
    for(size_t i=4;i<sliders.size();++i){ size_t j=i-4; int col=(int)(j%cols), row=(int)(j/cols); if(row*cellH>=ar.getHeight()) continue; sliders[i]->setBounds(ar.getX()+col*cellW+4,ar.getY()+row*cellH+26,cellW-8,cellH-30); }
    // Labels are placed immediately above their matching slider.
    for(auto* c : simplePage.getChildren()) if(auto* l=dynamic_cast<juce::Label*>(c)) { auto b=l->getBounds(); (void)b; }
    // The labels are child components; align each label to the control by creation order.
    auto placeLabels=[&](juce::Component& page, size_t start, size_t count){ juce::Array<juce::Label*> ls; for(auto* c:page.getChildren()) if(auto* l=dynamic_cast<juce::Label*>(c)) ls.add(l); for(int i=0;i<ls.size() && i<(int)count;++i){auto b=sliders[start+(size_t)i]->getBounds(); ls[i]->setBounds(b.getX(),b.getY()-24,b.getWidth(),22);} };
    placeLabels(simplePage,0,4); placeLabels(advancedPage,4,sliders.size()-4);
}

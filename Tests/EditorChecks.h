#pragma once
#include <juce_opengl/juce_opengl.h>
#include "Visualizer.h"
#include <fstream>
#include <iostream>

// Called on the message thread while a directly instantiated processor is fed
// by the test host. VST3's native editor wrapper has no JUCE children to inspect.
struct EditorChecks {
    juce::AudioProcessorEditor& editor;
    juce::File output = juce::File::getCurrentWorkingDirectory().getChildFile("Reports");
    fly::SharedState& shared;
    juce::AudioProcessorValueTreeState& parameters;
    std::uint32_t stoppedFrames = 0;
    std::atomic<bool> glPassed {false}, activityPassed {false}, disablePassed {false}, reenablePassed {false};
    std::atomic<bool> deltaPassed {false};
    std::atomic<int> phase {0};
    EditorChecks(juce::AudioProcessorEditor& e, fly::SharedState& s, juce::AudioProcessorValueTreeState& p)
        : editor(e), shared(s), parameters(p) { output.createDirectory(); }
    AuditoryVisualizer* visualizer() {
        for (int i = 0; i < editor.getNumChildComponents(); ++i)
            if (auto* v = dynamic_cast<AuditoryVisualizer*>(editor.getChildComponent(i))) return v;
        return nullptr;
    }
    bool click(const juce::String& text) {
        for (int i = 0; i < editor.getNumChildComponents(); ++i)
            if (auto* b = dynamic_cast<juce::ToggleButton*>(editor.getChildComponent(i)); b && b->getButtonText() == text) {
                b->triggerClick(); return true;
            }
        return false;
    }
    void schedule() {
        juce::Timer::callAfterDelay(2500, [this] {
            juce::PNGImageFormat png;
            for (int i=0;i<editor.getNumChildComponents();++i) {
                auto* child = editor.getChildComponent(i);
                if (auto* context = juce::OpenGLContext::getContextAttachedTo(*child)) {
                    const int width=child->getWidth(), height=child->getHeight();
                    context->executeOnGLThread([this, width, height](juce::OpenGLContext& current) {
                        using namespace juce::gl;
                        const auto scaling = current.getRenderingScale();
                        const int pixelsWide=juce::roundToInt(width*scaling), pixelsHigh=juce::roundToInt(height*scaling);
                        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(pixelsWide*pixelsHigh*4));
                        glReadPixels(0,0,pixelsWide,pixelsHigh,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data()); const auto error=glGetError();
                        juce::Image circuit(juce::Image::RGB,pixelsWide,pixelsHigh,true); int illuminated=0;
                        for(int y=0;y<pixelsHigh;++y) for(int x=0;x<pixelsWide;++x) {
                            const auto offset=static_cast<std::size_t>(((pixelsHigh-1-y)*pixelsWide+x)*4);
                            auto red=pixels[offset], green=pixels[offset+1], blue=pixels[offset+2];
                            if(red>40 || green>40 || blue>50) ++illuminated;
                            circuit.setPixelAt(x,y,juce::Colour(red,green,blue));
                        }
                        juce::PNGImageFormat format;
                        if(auto stream=output.getChildFile("circuit.png").createOutputStream()) { stream->setPosition(0); stream->truncate(); format.writeImageToStream(circuit,*stream); }
                        glPassed.store(error==GL_NO_ERROR && illuminated>1000);
                        std::cout << "OPENGL: error=" << error << ", illuminated pixels=" << illuminated << '\n';
                    },true);
                }
            }
            auto image=editor.createComponentSnapshot(editor.getLocalBounds());
            if(auto stream=output.getChildFile("editor.png").createOutputStream()) { stream->setPosition(0); stream->truncate(); png.writeImageToStream(image,*stream); }
            toggle(); click("Delta listen"); phase.store(1);
        });
        juce::Timer::callAfterDelay(3000,[this] {
            bool boundsOK = false;
            for (int i = 0; i < editor.getNumChildComponents(); ++i)
                if (auto* b = dynamic_cast<juce::ToggleButton*>(editor.getChildComponent(i)); b && b->getButtonText() == "Delta listen")
                    boundsOK = b->isShowing() && editor.getLocalBounds().contains(b->getBounds()) && b->getToggleState();
            deltaPassed.store(boundsOK && parameters.getRawParameterValue("delta")->load() == 1);
            click("Delta listen");
        });
        juce::Timer::callAfterDelay(3300,[this] {
            deltaPassed.store(deltaPassed.load() && parameters.getRawParameterValue("delta")->load() == 0);
            std::cout << "DELTA BUTTON: " << deltaPassed.load() << '\n';
        });
        juce::Timer::callAfterDelay(3500,[this] { click("Disable visualisation"); phase.store(2); });
        juce::Timer::callAfterDelay(4200,[this] { stoppedFrames = shared.telemetry.renderedFrames.load(); });
        juce::Timer::callAfterDelay(5200,[this] {
            auto* v = visualizer();
            disablePassed.store(v && !v->isRenderingEnabled() && !juce::OpenGLContext::getContextAttachedTo(*v)
                && !shared.visualActive.load() && !shared.visualizationEnabled.load()
                && shared.telemetry.renderedFrames.load() == stoppedFrames
                && parameters.getRawParameterValue("disableVisualization")->load() == 1
                && shared.telemetry.audioRates[4].load() > 0 && shared.telemetry.input.load() > 0);
            std::cout << "VISUALISATION DISABLED: " << disablePassed.load() << ", stopped frame count=" << stoppedFrames << '\n';
            editor.setSize(1020,900); toggle(); click("Disable visualisation"); phase.store(3);
        });
        juce::Timer::callAfterDelay(6500,[this] {
            auto* v = visualizer();
            reenablePassed.store(v && v->isRenderingEnabled() && juce::OpenGLContext::getContextAttachedTo(*v)
                && shared.visualActive.load() && shared.telemetry.renderedFrames.load() > stoppedFrames
                && parameters.getRawParameterValue("disableVisualization")->load() == 0);
            for (int i = 0; i < editor.getNumChildComponents(); ++i)
                if (auto* b = dynamic_cast<juce::ToggleButton*>(editor.getChildComponent(i)); b && b->getButtonText() == "Delta listen")
                    deltaPassed.store(deltaPassed.load() && b->isShowing() && editor.getLocalBounds().contains(b->getBounds()));
            std::cout << "VISUALISATION RE-ENABLED: " << reenablePassed.load() << ", Delta visible at minimum size=" << deltaPassed.load() << '\n'; phase.store(4);
        });
        juce::Timer::callAfterDelay(7500,[this] {
            juce::StringArray labels;
            for(int i=0;i<editor.getNumChildComponents();++i) if(auto* label=dynamic_cast<juce::Label*>(editor.getChildComponent(i))) labels.add(label->getText());
            const auto text=labels.joinIntoString("\n"); output.getChildFile("editor-status.txt").replaceWithText(text);
            activityPassed.store(text.contains("731 verified v783 neurons") && text.contains("Drosophila melanogaster") && text.contains("Lazirko Records")
                && text.contains("latest audio-control inputs") && !text.containsIgnoreCase("error") && !text.contains("JO  0.0 Hz"));
            std::cout << "LIVE EDITOR: " << labels.joinIntoString(" | ").toStdString() << '\n';
        });
    }
    void toggle() {
        for(int i=0;i<editor.getNumChildComponents();++i) if(auto* button=dynamic_cast<juce::ToggleButton*>(editor.getChildComponent(i)))
            if(button->getButtonText()=="FAFB anchor coordinates" || button->getButtonText()=="JO-B") button->triggerClick();
    }
};

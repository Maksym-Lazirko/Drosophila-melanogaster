#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_opengl/juce_opengl.h>
#include <array>
#include <chrono>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

void require(bool v, const char* message) { if (!v) throw std::runtime_error(message); }
static juce::AudioProcessorParameter* namedParameter(juce::AudioProcessor& processor, const char* name) {
    for (auto* p : processor.getParameters()) if (p->getName(128) == name) return p;
    throw std::runtime_error(std::string("VST3 parameter missing: ") + name);
}
static void hostControlAudit(juce::AudioPluginFormatManager& formats, const juce::PluginDescription& description) {
    // Actual bundled interface: parameters are changed via host automation, not
    // processor pointers. Compare endpoint recordings on a paced pulse/multitone.
    const auto render = [&](const char* name, const char* value, int mode) {
        juce::String error; auto p = formats.createPluginInstance(description, 48000, 512, error);
        if (!p) throw std::runtime_error(error.toStdString());
        const auto set = [&](const char* n, const char* v) { auto* parameter = namedParameter(*p, n); parameter->setValueNotifyingHost(parameter->getValueForText(v)); };
        set("Johnston's organ sensory gain", "4"); set("Neural influence", "4");
        namedParameter(*p, "Neural output mode")->setValueNotifyingHost(static_cast<float>(mode) / 3);
        if (name) set(name, value);
        p->setPlayConfigDetails(2, 2, 48000, 512); p->prepareToPlay(48000, 512);
        juce::AudioBuffer<float> b(2, 512); juce::MidiBuffer events; std::vector<float> recording;
        for (int block = 0; block < 180; ++block) {
            for (int i = 0; i < 512; ++i) {
                const auto t = static_cast<double>(block * 512 + i) / 48000;
                const auto envelope = std::fmod(t, 0.19) < 0.055 ? 1.0 : 0.35;
                const auto x = static_cast<float>(envelope * (0.28 * std::sin(6.283185307 * 150 * t) + 0.22 * std::sin(6.283185307 * 310 * t)
                    + 0.13 * std::sin(6.283185307 * 1100 * t) + 0.08 * std::sin(6.283185307 * 3600 * t)));
                b.setSample(0, i, x); b.setSample(1, i, -x);
            }
            p->processBlock(b, events);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) {
                const auto x = b.getSample(c, i); require(std::isfinite(x) && std::abs(x) < 16, "Actual VST3 control produced invalid audio");
                if (block >= 90) recording.push_back(x);
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10667));
        }
        p->releaseResources(); return recording;
    };
    const auto difference = [](const std::vector<float>& a, const std::vector<float>& b) {
        double energy = 0;
        for (std::size_t i = 0; i < a.size(); ++i) { const auto d = static_cast<double>(a[i]) - b[i]; energy += d * d; }
        return std::sqrt(energy / static_cast<double>(a.size()));
    };
    const auto baseline = render(nullptr, nullptr, 0);
    const auto repeatNoise = difference(baseline, render(nullptr, nullptr, 0));
    std::cout << "HOST baseline_repeat_difference_rms=" << repeatNoise << " (asynchronous scheduling is not deterministic)\n";
    struct Sweep { const char* name; const char* low; const char* high; };
    for (const auto& s : {Sweep{"Johnston's organ sensory gain", "0", "32"}, Sweep{"JO-B to JO-A balance", "0", "1"},
                         Sweep{"Simulation time-scale", "0.25", "4"}, Sweep{"JO band centre Hz", "60", "8000"},
                         Sweep{"JO band separation octaves", "0.5", "3"}, Sweep{"Wet / dry", "0", "1"}, Sweep{"Neural influence", "0", "4"}}) {
        const auto low = render(s.name, s.low, 0), high = render(s.name, s.high, 0);
        const auto rms = difference(low, high);
        require(rms > 0.00001, "Actual VST3 audio control has no endpoint response");
        std::cout << "HOST CONTROL " << s.name << " low=" << s.low << " high=" << s.high << " difference_rms=" << rms << '\n';
    }
    std::array<std::vector<float>, 4> modes;
    modes[0] = baseline;
    for (int mode = 1; mode < 4; ++mode) modes[static_cast<std::size_t>(mode)] = render(nullptr, nullptr, mode);
    for (int a = 0; a < 4; ++a) for (int b = a + 1; b < 4; ++b) {
        const auto rms = difference(modes[static_cast<std::size_t>(a)], modes[static_cast<std::size_t>(b)]);
        require(rms > 0.00001, "Actual VST3 modes have identical outputs");
        std::cout << "HOST MODE " << a << '-' << b << " difference_rms=" << rms << '\n';
    }
    std::cout << "PASS: actual VST3 all audio-control endpoints and all mode pairs at artistic drive/influence, finite stereo output\n";
}
int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    try {
        require(argc >= 2, "Pass actual VST3 bundle path");
        juce::VST3PluginFormat format; juce::OwnedArray<juce::PluginDescription> types;
        format.findAllTypesForFile(types, juce::File(juce::String(argv[1])).getFullPathName());
        require(types.size() == 1, "Actual VST3 did not scan as one effect");
        require(types[0]->name == "Drosophila melanogaster" && types[0]->manufacturerName == "Lazirko Records", "VST3 product/company metadata incorrect");
        juce::AudioPluginFormatManager formats; formats.addFormat(std::make_unique<juce::VST3PluginFormat>());
        juce::String error; auto plugin = formats.createPluginInstance(*types[0], 48000, 128, error);
        if (!plugin) throw std::runtime_error("VST3 instantiation failed: " + error.toStdString());
        for (const auto& entry : {std::pair<const char*, const char*>{"Johnston's organ sensory gain", "1"}, {"JO-B to JO-A balance", "0.5"},
                 {"Simulation time-scale", "1"}, {"JO band centre Hz", "300"}, {"JO band separation octaves", "1.5"},
                 {"Wet / dry", "1"}, {"Neural influence", "1"}, {"Spike brightness", "1.5"}, {"Spike decay seconds", "0.18"}}) {
            auto* value = namedParameter(*plugin, entry.first);
            require(std::abs(value->getDefaultValue() - value->getValueForText(entry.second)) < 0.00001f, "Actual VST3 factory default not model baseline");
            require(std::abs(value->getValue() - value->getDefaultValue()) < 0.00001f, "Actual VST3 initial value differs from factory default");
        }
        require(namedParameter(*plugin, "Neural output mode")->getValue() == 0, "Actual VST3 does not start in clean mode");
        if (argc > 2 && juce::String(argv[2]) == "--controls") {
            plugin.reset(); hostControlAudit(formats, *types[0]); return 0;
        }
        plugin->setPlayConfigDetails(2, 2, 48000, 128); plugin->prepareToPlay(48000, 128);
        juce::AudioBuffer<float> buffer(2, 128); juce::MidiBuffer midi; double energy = 0; bool changed = false;
        for (int k = 0; k < 750; ++k) {
            for (int i = 0; i < 128; ++i) { const float value = 0.4f * std::sin(6.2831853f * 220.0f * static_cast<float>(k * 128 + i) / 48000); buffer.setSample(0, i, value); buffer.setSample(1, i, -value); }
            const auto before = buffer.getSample(0, 32); plugin->processBlock(buffer, midi);
            changed |= std::abs(before - buffer.getSample(0, 32)) > 0.0001f;
            for (int i = 0; i < 128; ++i) { const auto v = buffer.getSample(0, i); require(std::isfinite(v), "VST3 nonfinite output"); energy += v * v; }
            std::this_thread::sleep_for(std::chrono::microseconds(2667));
        }
        require(changed && energy > 1, "Actual VST3 does not process audio");
        juce::MemoryBlock state; plugin->getStateInformation(state); require(state.getSize() > 0, "VST3 state missing"); plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        juce::AudioProcessorParameter* delta = nullptr;
        juce::AudioProcessorParameter* wet = nullptr;
        juce::AudioProcessorParameter* influence = nullptr;
        for (auto* parameter : plugin->getParameters()) {
            if (parameter->getName(128) == "Delta listen") delta = parameter;
            if (parameter->getName(128) == "Wet / dry") wet = parameter;
            if (parameter->getName(128) == "Neural influence") influence = parameter;
        }
        require(delta && wet && influence, "Actual VST3 missing Delta or mix parameters");
        require(delta->getValue() == 0 && delta->isAutomatable(), "Delta default/automation incorrect");
        delta->setValueNotifyingHost(1);
        juce::MemoryBlock deltaState; plugin->getStateInformation(deltaState);
        delta->setValueNotifyingHost(0);
        plugin->setStateInformation(deltaState.getData(), static_cast<int>(deltaState.getSize()));
        require(delta->getValue() == 1, "Actual VST3 did not recall Delta state");
        bool deltaAudible = false;
        for (int k = 0; k < 100; ++k) {
            for (int i = 0; i < 128; ++i) {
                const auto x = 0.4f * std::sin(6.2831853f * 220.0f * static_cast<float>((k + 750) * 128 + i) / 48000);
                buffer.setSample(0, i, x); buffer.setSample(1, i, -x);
            }
            plugin->processBlock(buffer, midi);
            for (int i = 0; i < 128; ++i) {
                require(std::isfinite(buffer.getSample(0, i)), "Actual VST3 Delta is nonfinite");
                deltaAudible |= std::abs(buffer.getSample(0, i)) > 0.00001f;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(2667));
        }
        require(deltaAudible, "Actual VST3 Delta produced no residual");
        wet->setValueNotifyingHost(0); influence->setValueNotifyingHost(0);
        // Reprepare clears wet/depth smoothing and filter state, then the Delta
        // ramp settles against an exactly dry carrier. Both channels must null.
        plugin->releaseResources(); plugin->prepareToPlay(48000, 128);
        for (int k = 0; k < 12; ++k) {
            for (int i = 0; i < 128; ++i) { buffer.setSample(0, i, 0.2f); buffer.setSample(1, i, -0.3f); }
            plugin->processBlock(buffer, midi);
            if (k >= 4) for (int c = 0; c < 2; ++c) for (int i = 0; i < 128; ++i)
                require(buffer.getSample(c, i) == 0, "Actual VST3 dry Delta does not null");
            std::this_thread::sleep_for(std::chrono::microseconds(2667));
        }
        delta->setValueNotifyingHost(0);
        for (int k = 0; k < 12; ++k) {
            for (int i = 0; i < 128; ++i) { buffer.setSample(0, i, 0.2f); buffer.setSample(1, i, -0.3f); }
            plugin->processBlock(buffer, midi);
            if (k >= 4) for (int i = 0; i < 128; ++i)
                require(buffer.getSample(0, i) == 0.2f && buffer.getSample(1, i) == -0.3f, "Delta disable did not restore VST3 dry audio");
            std::this_thread::sleep_for(std::chrono::microseconds(2667));
        }
        plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        std::cout << "PASS: actual VST3 Delta automation, recall, active residual, stereo null and normal-output restoration\n";
        if (argc > 2 && juce::String(argv[2]) == "--editor") {
            auto editor = std::unique_ptr<juce::AudioProcessorEditor>(plugin->createEditorIfNeeded()); require(editor != nullptr, "VST3 editor missing");
            editor->addToDesktop(juce::ComponentPeer::windowIsTemporary); editor->setVisible(true);
            juce::File output = juce::File::getCurrentWorkingDirectory().getChildFile("Reports"); output.createDirectory();
            juce::File wav = output.getChildFile("host-output.wav"); std::ofstream recording(wav.getFullPathName().toStdString(), std::ios::binary);
            const auto write16 = [&recording](std::uint16_t x) { char b[2] {static_cast<char>(x), static_cast<char>(x >> 8)}; recording.write(b, 2); };
            const auto write32 = [&recording](std::uint32_t x) { char b[4]; for(int i=0;i<4;++i)b[i]=static_cast<char>(x>>(8*i)); recording.write(b,4); };
            recording.write("RIFF",4); write32(0); recording.write("WAVEfmt ",8); write32(16); write16(1); write16(2); write32(48000); write32(192000); write16(4); write16(16); recording.write("data",4); write32(0);
            std::atomic<bool> quit {false}; std::uint32_t recordedBytes = 0;
            std::thread audio([&] { juce::AudioBuffer<float> b(2,128); juce::MidiBuffer events; std::uint64_t offset = 0;
                while (!quit.load()) {
                    for(int i=0;i<128;++i) { const auto t = static_cast<double>(offset++) / 48000;
                        const auto envelope = (static_cast<int>(t) % 3) == 0 ? 0.2f : (std::fmod(t,0.036)<0.010 ? 0.5f : 0.01f);
                        const auto v = envelope * static_cast<float>(std::sin(6.283185307 * 220 * t)); b.setSample(0,i,v); b.setSample(1,i,v); }
                    plugin->processBlock(b,events);
                    for(int i=0;i<128;++i) for(int c=0;c<2;++c) write16(static_cast<std::uint16_t>(static_cast<std::int16_t>(std::clamp(b.getSample(c,i),-1.0f,1.0f)*32767)));
                    recordedBytes += 128 * 4; std::this_thread::sleep_for(std::chrono::microseconds(2667));
                } });
            std::atomic<bool> editorCaptured {false};
            juce::Timer::callAfterDelay(2500, [&] {
                auto image = editor->createComponentSnapshot(editor->getLocalBounds()); juce::PNGImageFormat png;
                // A hosted VST3 exposes a native wrapper, not the plugin's
                // component tree. Direct processor tests inspect its GL renderer.
                if (auto stream = output.getChildFile("native-vst3-wrapper.png").createOutputStream()) { stream->setPosition(0); stream->truncate(); png.writeImageToStream(image,*stream); }
                editorCaptured.store(editor->isShowing() && image.isValid() && editor->getWidth() >= 1020);
            });
            juce::Timer::callAfterDelay(5000, [&] {
                editor->setSize(1020,900);
            });
            std::thread stopLoop([] { std::this_thread::sleep_for(std::chrono::seconds(10)); juce::MessageManager::getInstance()->stopDispatchLoop(); });
            juce::MessageManager::getInstance()->runDispatchLoop(); stopLoop.join(); quit.store(true); audio.join();
            recording.seekp(4); write32(recordedBytes+36); recording.seekp(40); write32(recordedBytes); recording.close();
            require(editorCaptured.load(),"Native VST3 editor did not show or capture");
            editor.reset();
        }
        plugin->releaseResources();
        std::cout << "PASS: actual VST3 bundle scanned, instantiated, processed stereo audio, recalled state" << (argc>2?" and exercised live OpenGL editor":"") << '\n'; return 0;
    } catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}

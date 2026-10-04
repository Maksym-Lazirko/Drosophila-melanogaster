#pragma once
#include <juce_opengl/juce_opengl.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "RealtimeTransport.h"

class AuditoryVisualizer final : public juce::Component, private juce::OpenGLRenderer, private juce::Timer {
public:
    AuditoryVisualizer(const fly::Graph&, fly::SharedState&, juce::AudioProcessorValueTreeState&);
    ~AuditoryVisualizer() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    juce::String rendererError() const;
    void setRenderingEnabled(bool); // message thread: detach actually stops GPU work
    bool isRenderingEnabled() const noexcept { return renderingEnabled; }
private:
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
    void timerCallback() override { context.triggerRepaint(); }
    struct Vertex { float x, y, r, g, b, a, u, v; };
    void draw(const std::vector<Vertex>&, bool circles);
    void addVertex(std::vector<Vertex>&, juce::Point<float>, juce::Colour, float alpha, float u = 0, float v = 0);
    const fly::Graph& graph;
    fly::SharedState& shared;
    juce::OpenGLContext context;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> position, colour, uv;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> circle;
    unsigned int buffer = 0, vao = 0;
    std::vector<juce::Point<float>> schematic, spatial, transformed;
    std::vector<float> flash;
    std::vector<Vertex> nodes, lines;
    std::array<std::atomic<float>*, fly::populationCount> show {};
    std::atomic<float> *brightness, *decay, *connections, *realCoordinates, *colourMode;
    std::atomic<float> zoom {1}, panX {0}, panY {0};
    std::atomic<int> width {1}, height {1};
    juce::Point<float> dragStart, initialPan;
    double lastFrame = 0;
    bool renderingEnabled = false;
    mutable juce::CriticalSection errorLock;
    juce::String error;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuditoryVisualizer)
};

#include "Visualizer.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace {
const std::array<juce::Colour, fly::populationCount> palette {
    juce::Colour(0xffffbc63), juce::Colour(0xff71e5dd), juce::Colour(0xfff287bc),
    juce::Colour(0xffbba0ff), juce::Colour(0xff98f18c), juce::Colour(0xfff1e17d), juce::Colour(0xff81b8ff) };
}
AuditoryVisualizer::AuditoryVisualizer(const fly::Graph& g, fly::SharedState& s, juce::AudioProcessorValueTreeState& p)
    : graph(g), shared(s) {
    brightness = p.getRawParameterValue("brightness"); decay = p.getRawParameterValue("decay");
    connections = p.getRawParameterValue("connections"); realCoordinates = p.getRawParameterValue("spatial"); colourMode = p.getRawParameterValue("colour");
    for (std::size_t i = 0; i < show.size(); ++i) show[i] = p.getRawParameterValue("show" + juce::String(static_cast<int>(i)));
    const auto n = graph.neurons.size(); schematic.resize(n); spatial.resize(n); transformed.resize(n); flash.assign(n, 0);
    nodes.reserve(n * 6); lines.reserve(graph.synapses.size() * 2);
    std::array<std::size_t, fly::populationCount> ordinal {};
    float minX = 1.0e10f, maxX = -1.0e10f, minZ = minX, maxZ = maxX;
    for (const auto& v : graph.neurons) { minX = std::min(minX, v.x); maxX = std::max(maxX, v.x); minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z); }
    const auto scale = std::max({maxX - minX, maxZ - minZ, 1.0f});
    for (std::size_t i = 0; i < n; ++i) {
        const auto& v = graph.neurons[i]; const auto pIndex = v.population;
        const auto k = ordinal[pIndex]++; const float cols = std::ceil(std::sqrt(static_cast<float>(graph.counts[pIndex]) * 0.42f));
        const auto column = static_cast<float>(k % static_cast<std::size_t>(std::max(1.0f, cols)));
        const auto row = static_cast<float>(k / static_cast<std::size_t>(std::max(1.0f, cols)));
        const float rows = std::ceil(static_cast<float>(graph.counts[pIndex]) / std::max(1.0f, cols));
        float x = pIndex < 2 ? -0.68f : pIndex < 6 ? -0.03f : 0.64f;
        const float yCentre = pIndex == 0 ? 0.40f : pIndex == 1 ? -0.40f : pIndex < 6 ? 0.60f - 0.40f * static_cast<float>(pIndex - 2) : 0;
        schematic[i] = {x + (column - (cols - 1) * 0.5f) * 0.022f,
                        yCentre + (row - (rows - 1) * 0.5f) * 0.022f};
        spatial[i] = {(v.x - (maxX + minX) * 0.5f) * 1.7f / scale, -(v.z - (maxZ + minZ) * 0.5f) * 1.7f / scale};
    }
    setOpaque(true); context.setRenderer(this); context.setComponentPaintingEnabled(false);
    context.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2); context.setContinuousRepainting(false);
    setRenderingEnabled(p.getRawParameterValue("disableVisualization")->load() < 0.5f);
}
AuditoryVisualizer::~AuditoryVisualizer() { setRenderingEnabled(false); }
void AuditoryVisualizer::setRenderingEnabled(bool enabled) {
    shared.visualizationEnabled.store(enabled, std::memory_order_release);
    if (enabled == renderingEnabled) return;
    renderingEnabled = enabled;
    if (!enabled) {
        stopTimer(); shared.visualActive.store(false, std::memory_order_release); context.detach();
    }
    // Context is now detached, or not yet attached: no GL consumer can race.
    fly::SpikeEvent stale; while (shared.spikes.pop(stale)) {}
    std::fill(flash.begin(), flash.end(), 0.0f);
    if (enabled) { context.attachTo(*this); startTimerHz(60); }
    repaint();
}
juce::String AuditoryVisualizer::rendererError() const { const juce::ScopedLock lock(errorLock); return error; }
void AuditoryVisualizer::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0a1120));
    if (!renderingEnabled) {
        g.setColour(juce::Colour(0xff91b6ba)); g.setFont(juce::FontOptions(18.0f));
        g.drawText("Visualisation disabled | Auditory simulation and audio remain active", getLocalBounds().reduced(20), juce::Justification::centred, true);
    }
}
void AuditoryVisualizer::resized() { width.store(getWidth()); height.store(getHeight()); }
void AuditoryVisualizer::newOpenGLContextCreated() {
    using namespace juce::gl;
    shader = std::make_unique<juce::OpenGLShaderProgram>(context);
    const auto vertex = R"(attribute vec2 position; attribute vec4 colour; attribute vec2 uv;
        varying vec4 tint; varying vec2 local; void main() { tint=colour; local=uv; gl_Position=vec4(position,0.0,1.0); })";
    const auto fragment = R"(varying vec4 tint; varying vec2 local; uniform int circle;
        void main() { float alpha=tint.a; if(circle==1) { float r=length(local); if(r>1.0) discard;
        alpha *= 1.0-smoothstep(0.35,1.0,r); } gl_FragColor=vec4(tint.rgb,alpha); })";
    if (!shader->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(vertex))
        || !shader->addFragmentShader(juce::OpenGLHelpers::translateFragmentShaderToV3(fragment)) || !shader->link()) {
        const juce::ScopedLock lock(errorLock); error = shader->getLastError(); shader.reset(); return;
    }
    position = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "position");
    colour = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "colour");
    uv = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "uv");
    circle = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader, "circle");
    glGenVertexArrays(1, &vao); glGenBuffers(1, &buffer); context.setSwapInterval(1);
    lastFrame = juce::Time::getMillisecondCounterHiRes();
    shared.visualActive.store(shared.visualizationEnabled.load(std::memory_order_acquire), std::memory_order_release);
}
void AuditoryVisualizer::addVertex(std::vector<Vertex>& vertices, juce::Point<float> p, juce::Colour c, float alpha, float u, float v) {
    vertices.push_back({p.x, p.y, c.getFloatRed(), c.getFloatGreen(), c.getFloatBlue(), alpha, u, v});
}
void AuditoryVisualizer::draw(const std::vector<Vertex>& vertices, bool circles) {
    using namespace juce::gl;
    if (vertices.empty()) return;
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STREAM_DRAW);
    const auto attribute = [](int id, int count, std::size_t offset) {
        glVertexAttribPointer(static_cast<GLuint>(id), count, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offset));
        glEnableVertexAttribArray(static_cast<GLuint>(id)); };
    attribute(position->attributeID, 2, offsetof(Vertex, x)); attribute(colour->attributeID, 4, offsetof(Vertex, r)); attribute(uv->attributeID, 2, offsetof(Vertex, u));
    circle->set(circles ? 1 : 0); glDrawArrays(circles ? GL_TRIANGLES : GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
    glBindBuffer(GL_ARRAY_BUFFER, 0); glBindVertexArray(0);
}
void AuditoryVisualizer::renderOpenGL() {
    using namespace juce::gl;
    if (!shared.visualizationEnabled.load(std::memory_order_acquire)) return;
    shared.telemetry.renderedFrames.fetch_add(1, std::memory_order_relaxed);
    juce::OpenGLHelpers::clear(juce::Colour(0xff080f1c)); if (!shader) return;
    const int w = std::max(1, width.load()), h = std::max(1, height.load());
    const auto renderScale = context.getRenderingScale(); glViewport(0, 0, juce::roundToInt(w * renderScale), juce::roundToInt(h * renderScale));
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = static_cast<float>(std::clamp((now - lastFrame) * 0.001, 0.0, 0.1)); lastFrame = now;
    const float fade = std::exp(-dt / std::max(0.03f, decay->load()));
    for (auto& f : flash) f *= fade;
    fly::SpikeEvent event; int drained = 0;
    while (drained++ < 65536 && shared.spikes.pop(event)) if (event.neuron < flash.size()) flash[event.neuron] = 1;
    const auto& layout = realCoordinates->load() > 0.5f ? spatial : schematic;
    const float z = zoom.load(), x = panX.load(), y = panY.load(), aspect = static_cast<float>(h) / static_cast<float>(w);
    for (std::size_t i = 0; i < layout.size(); ++i) transformed[i] = {layout[i].x * z * aspect + x, layout[i].y * z + y};
    std::array<bool, fly::populationCount> visible {}; for (std::size_t p = 0; p < visible.size(); ++p) visible[p] = show[p]->load() > 0.5f;
    nodes.clear(); lines.clear(); const auto colourBy = static_cast<int>(colourMode->load());
    const float bright = brightness->load();
    if (connections->load() > 0.5f) for (std::size_t i = 0; i < graph.neurons.size(); ++i) {
        if (!visible[graph.neurons[i].population]) continue;
        for (auto edge = graph.offsets[i]; edge < graph.offsets[i + 1]; ++edge) {
            const auto target = graph.synapses[edge].target; if (!visible[graph.neurons[target].population]) continue;
            const auto weight = graph.synapses[edge].weightMV;
            const auto tint = weight == 0 ? juce::Colour(0xff68717c) : weight < 0 ? juce::Colour(0xffce75bf) : juce::Colour(0xff5ecbb6);
            // Unknown/neuromodulatory zero-fast-weight edges do not transmit in
            // this LIF model: draw dim structural lines, never false flashes.
            const float alpha = weight == 0 ? 0.002f : std::min(0.16f, 0.004f + flash[i] * 0.045f * bright);
            addVertex(lines, transformed[i], tint, alpha); addVertex(lines, transformed[target], tint, alpha * 0.5f);
        }
    }
    constexpr float corners[6][2] {{-1,-1},{1,-1},{1,1},{-1,-1},{1,1},{-1,1}};
    for (std::size_t i = 0; i < graph.neurons.size(); ++i) {
        const auto& v = graph.neurons[i]; if (!visible[v.population]) continue;
        auto tint = palette[v.population];
        if (colourBy == 1) tint = v.neurotransmitter == 1 ? juce::Colour(0xff72ebcb) : v.neurotransmitter == 2 ? juce::Colour(0xffed86ba) : v.neurotransmitter == 3 ? juce::Colour(0xffffba65) : juce::Colour(0xffa39acd);
        if (colourBy == 2) tint = juce::Colour(0xff426185).interpolatedWith(juce::Colour(0xffffeeaa), flash[i]);
        const float radius = (2.4f + flash[i] * 3.5f) * std::sqrt(z);
        const float alpha = std::clamp(0.30f + flash[i] * bright, 0.0f, 1.0f);
        for (const auto& corner : corners) addVertex(nodes, {transformed[i].x + corner[0] * radius * 2 / static_cast<float>(w), transformed[i].y + corner[1] * radius * 2 / static_cast<float>(h)}, tint, alpha, corner[0], corner[1]);
    }
    shader->use(); draw(lines, false); draw(nodes, true); glUseProgram(0); glDisable(GL_BLEND);
}
void AuditoryVisualizer::openGLContextClosing() {
    using namespace juce::gl;
    shared.visualActive.store(false, std::memory_order_release);
    if (buffer) glDeleteBuffers(1, &buffer); if (vao) glDeleteVertexArrays(1, &vao); buffer = vao = 0;
    circle.reset(); uv.reset(); colour.reset(); position.reset(); shader.reset();
}
void AuditoryVisualizer::mouseDown(const juce::MouseEvent& e) { dragStart = e.position; initialPan = {panX.load(), panY.load()}; }
void AuditoryVisualizer::mouseDrag(const juce::MouseEvent& e) {
    const auto d = e.position - dragStart;
    panX.store(initialPan.x + 2 * d.x / static_cast<float>(std::max(1, getWidth())));
    panY.store(initialPan.y - 2 * d.y / static_cast<float>(std::max(1, getHeight())));
}
void AuditoryVisualizer::mouseDoubleClick(const juce::MouseEvent&) { zoom.store(1); panX.store(0); panY.store(0); }
void AuditoryVisualizer::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) { zoom.store(std::clamp(zoom.load() * std::exp(wheel.deltaY * 1.7f), 0.35f, 6.0f)); }

// SFML 3 + OpenGL application.
// Geometry + Lighting: delegated to Renderer / OctaMesh; this file is UI, input, and puzzle state routing.

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <algorithm>
#include <ctime>
#include <optional>
#include <string>

#include "math3.h"
#include "octa_mesh.h"
#include "octa_state.h"
#include "renderer.h"
#include "window_shape.h"

namespace {

constexpr unsigned kWindowW = 900;
constexpr unsigned kWindowH = 900;
constexpr float kAnimDegPerSec = 320.f;
constexpr float kMeshViewScale = 0.70f;

float g_hudChromeBottomY = 120.f;

enum class LeftDragKind : std::uint8_t { None = 0, Orbit, MoveWindow };

void startFaceTurn(const OctaMove& m, TurnAnimOcta& anim, OctaMove& pending) {
    if (anim.active) {
        return;
    }
    pending = m;
    anim.active = true;
    anim.currentDeg = 0.f;
    anim.face = m.face;
    anim.depth = static_cast<int>(m.depth);
    anim.dir = static_cast<int>(m.dir);
}

void advanceTurnAnimation(float dt, TurnAnimOcta& anim, OctaState& state, OctaMove& pending) {
    if (!anim.active) {
        return;
    }
    anim.currentDeg += kAnimDegPerSec * dt;
    if (anim.currentDeg >= kOctaTurnDeg) {
        anim.currentDeg = kOctaTurnDeg;
        state.apply(pending);
        anim.active = false;
        anim.currentDeg = 0.f;
    }
}

void applyOrbitDrag(float& yawDeg, float& pitchDeg, int dx, int dy) {
    constexpr float kOrbitSensitivity = 0.35f;
    yawDeg += static_cast<float>(dx) * kOrbitSensitivity;
    pitchDeg += static_cast<float>(dy) * kOrbitSensitivity;
    if (pitchDeg > 89.f) {
        pitchDeg = 89.f;
    }
    if (pitchDeg < -89.f) {
        pitchDeg = -89.f;
    }
}

sf::ContextSettings makeGlContextSettings() {
    sf::ContextSettings s;
    s.depthBits = 24;
    s.stencilBits = 8;
    s.majorVersion = 2;
    s.minorVersion = 1;
    return s;
}

bool tryLoadHudFont(sf::Font& fontOut, std::optional<sf::Text>& hudOut, std::optional<sf::Text>& devHudOut) {
    const bool ok = fontOut.openFromFile(R"(C:\Windows\Fonts\arial.ttf)") ||
                    fontOut.openFromFile(R"(C:\Windows\Fonts\calibri.ttf)");
    if (!ok) {
        return false;
    }
    constexpr unsigned kHudFontPx = 18u;
    hudOut.emplace(fontOut, "", kHudFontPx);
    // Aesthetic: HUD help text in very light sky blue (#E0F6FF).
    hudOut->setFillColor(sf::Color(224, 246, 255));
    hudOut->setLineSpacing(1.08f);
    constexpr unsigned kDevHudFontPx = 16u;
    devHudOut.emplace(fontOut, "", kDevHudFontPx);
    // Aesthetic: dev HUD default in powder blue (#B0E0E6).
    devHudOut->setFillColor(sf::Color(176, 224, 230));
    devHudOut->setLineSpacing(1.06f);
    return true;
}

void drawHudOverlay(sf::RenderWindow& window, sf::Text& hud) {
    hud.setString(std::string("1-8: face turn\n"
                              "Shift: reverse\n"
                              "Space: scramble\n"
                              "Tab: solve\n\n"
                              "0: ksticker\n"));
    const sf::FloatRect lb = hud.getLocalBounds();
    const float cw = static_cast<float>(window.getSize().x);
    constexpr float kHudPadX = 30.f;
    const float yHud = std::max(8.f, 24.f);
    hud.setOrigin({lb.position.x + lb.size.x * 0.5f, lb.position.y});
    hud.setPosition({cw * 0.5f, yHud});
    g_hudChromeBottomY = yHud + lb.size.y + 10.f;
    window.pushGLStates();
    window.draw(hud);
    window.popGLStates();
}

void drawDevNamesOnOcta(sf::RenderWindow& window, Renderer& renderer, const OctaMesh& mesh, float yawDeg,
                        float pitchDeg, float rollDeg, float meshScale, const TurnAnimOcta& anim, sf::Text& devHud) {
    constexpr unsigned kStickerIdxFontPx = 28u;
    constexpr unsigned kDevHudDefaultPx = 16u;

    const int vw = static_cast<int>(window.getSize().x);
    const int vh = static_cast<int>(window.getSize().y);
    const TurnAnimOcta* animPtr = anim.active ? &anim : nullptr;

    window.pushGLStates();
    devHud.setCharacterSize(kStickerIdxFontPx);

    // UI: while a face turn runs, fade ksticker labels on all other faces (ramp with turn angle).
    constexpr std::uint8_t kLabelAlphaFull = 255u;
    // number fade
    constexpr std::uint8_t kLabelAlphaFaded = 1u;
    float otherFaceFade = 0.f;
    if (anim.active) {
        otherFaceFade = std::clamp(anim.currentDeg / kOctaTurnDeg, 0.f, 1.f);
    }

    for (int fi = 0; fi < kOctaFaces; ++fi) {
        const OctaFace face = static_cast<OctaFace>(fi);
        if (!renderer.faceFacesCamera(face, yawDeg, pitchDeg, rollDeg)) {
            continue;
        }
        for (int r = 0; r < kLatticeOrder; ++r) {
            for (int c = 0; c < kLatticeOrder; ++c) {
                const int slot = meshStickerSlot(face, r, c);
                const Vec3 p = mesh.centroids[static_cast<std::size_t>(slot)];
                devHud.setString(std::to_string(slot + 1));
                double px = 0.0;
                double py = 0.0;
                bool vis = false;
                static_cast<void>(renderer.projectMeshAnchorToWindow(meshScale, yawDeg, pitchDeg, rollDeg, p, animPtr,
                                                                     vw, vh, px, py, vis, slot));
                if (!vis) {
                    continue;
                }
                // Aesthetic: turning face labels stay black; other faces fade during the move.
                if (anim.active && face != anim.face) {
                    const float alphaF = static_cast<float>(kLabelAlphaFull) +
                                         otherFaceFade *
                                             (static_cast<float>(kLabelAlphaFaded) - static_cast<float>(kLabelAlphaFull));
                    devHud.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alphaF)));
                } else {
                    devHud.setFillColor(sf::Color(0, 0, 0, kLabelAlphaFull));
                }
                const sf::FloatRect lb = devHud.getLocalBounds();
                devHud.setOrigin({lb.position.x + lb.size.x * 0.5f, lb.position.y + lb.size.y * 0.5f});
                devHud.setPosition({static_cast<float>(px), static_cast<float>(py)});
                window.draw(devHud);
            }
        }
    }
    devHud.setCharacterSize(kDevHudDefaultPx);
    window.popGLStates();
}

bool isPointerInHudDragStrip(sf::Vector2i p, const sf::RenderWindow& window) {
    const int h = static_cast<int>(window.getSize().y);
    const int stripH = std::min(160, std::max(56, h / 4));
    return p.y >= 0 && p.y < stripH && p.x >= 0 && p.x < static_cast<int>(window.getSize().x);
}

void handleGlobalPuzzleCommands(sf::Keyboard::Key key, OctaState& state, TurnAnimOcta& anim, sf::Clock& scrambleClock) {
    using K = sf::Keyboard::Key;
    if (key == K::Space) {
        if (!anim.active) {
            state.scramble(48, static_cast<unsigned>(scrambleClock.getElapsedTime().asMilliseconds()));
        }
    } else if (key == K::Backspace) {
        if (!anim.active) {
            state.undo();
        }
    } else if (key == K::Tab) {
        if (!anim.active) {
            state.autoComplete();
        }
    } else if (key == K::Enter) {
        anim.active = false;
        anim.currentDeg = 0.f;
        state.reset();
    }
}

bool trySetDepth(sf::Keyboard::Key key, bool ctrlHeld, int& depthOut) {
    if (!ctrlHeld) {
        return false;
    }
    using K = sf::Keyboard::Key;
    if (key == K::Numpad1 || key == K::Num1) {
        depthOut = 1;
    } else if (key == K::Numpad2 || key == K::Num2) {
        depthOut = 2;
    } else if (key == K::Numpad3 || key == K::Num3) {
        depthOut = 3;
    } else if (key == K::Numpad4 || key == K::Num4) {
        depthOut = 4;
    } else if (key == K::Numpad5 || key == K::Num5) {
        depthOut = 5;
    } else if (key == K::Numpad6 || key == K::Num6) {
        depthOut = 6;
    } else {
        return false;
    }
    return true;
}

void handlePuzzleKeys(sf::Keyboard::Key key, int& depth, int dir, OctaState& state, TurnAnimOcta& anim,
                      sf::Clock& scrambleClock, OctaMove& pending) {
    using K = sf::Keyboard::Key;
    const bool ctrlHeld =
        sf::Keyboard::isKeyPressed(K::LControl) || sf::Keyboard::isKeyPressed(K::RControl);
    if (trySetDepth(key, ctrlHeld, depth)) {
        return;
    }
    OctaMove m{};
    m.depth = static_cast<std::uint8_t>(depth);
    m.dir = static_cast<std::int8_t>(dir);
    if (key == K::Num1 || key == K::Numpad1) {
        m.face = OctaFace::Face1;
    } else if (key == K::Num2 || key == K::Numpad2) {
        m.face = OctaFace::Face2;
    } else if (key == K::Num3 || key == K::Numpad3) {
        m.face = OctaFace::Face3;
    } else if (key == K::Num4 || key == K::Numpad4) {
        m.face = OctaFace::Face4;
    } else if (key == K::Num5 || key == K::Numpad5) {
        m.face = OctaFace::Face5;
    } else if (key == K::Num6 || key == K::Numpad6) {
        m.face = OctaFace::Face6;
    } else if (key == K::Num7 || key == K::Numpad7) {
        m.face = OctaFace::Face7;
    } else if (key == K::Num8 || key == K::Numpad8) {
        m.face = OctaFace::Face8;
    } else {
        handleGlobalPuzzleCommands(key, state, anim, scrambleClock);
        return;
    }
    startFaceTurn(m, anim, pending);
}

void handleResizeEvent(const sf::Event::Resized& rs, sf::RenderWindow& window, Renderer& renderer) {
    renderer.resize(static_cast<int>(rs.size.x), static_cast<int>(rs.size.y));
    applySquareWindowShape(window);
}

void handleMousePressed(const sf::Event::MouseButtonPressed& mb, sf::RenderWindow& window, LeftDragKind& leftDrag,
                        sf::Vector2i& dragPos, sf::Vector2i& windowDragGrab) {
    if (mb.button != sf::Mouse::Button::Left) {
        return;
    }
    const bool altHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) ||
                         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);
    if (altHeld || isPointerInHudDragStrip(mb.position, window)) {
        leftDrag = LeftDragKind::MoveWindow;
        windowDragGrab = sf::Mouse::getPosition() - window.getPosition();
    } else {
        leftDrag = LeftDragKind::Orbit;
    }
    dragPos = mb.position;
}

void handleMouseReleased(const sf::Event::MouseButtonReleased& mr, LeftDragKind& leftDrag) {
    if (mr.button == sf::Mouse::Button::Left) {
        leftDrag = LeftDragKind::None;
    }
}

void handleMouseMoved(const sf::Event::MouseMoved& mm, sf::RenderWindow& window, LeftDragKind leftDrag,
                      sf::Vector2i& dragPos, sf::Vector2i windowDragGrab, float& yawDeg, float& pitchDeg) {
    if (leftDrag == LeftDragKind::MoveWindow) {
        window.setPosition(sf::Mouse::getPosition() - windowDragGrab);
    } else if (leftDrag == LeftDragKind::Orbit) {
        const int dx = mm.position.x - dragPos.x;
        const int dy = mm.position.y - dragPos.y;
        dragPos = mm.position;
        applyOrbitDrag(yawDeg, pitchDeg, dx, dy);
    }
}

void handleWheelScrolled(const sf::Event::MouseWheelScrolled& wheel, Renderer& renderer) {
    renderer.handleMouseWheel(static_cast<int>(wheel.delta));
}

void handleKeyPressed(const sf::Event::KeyPressed& kp, sf::RenderWindow& window, OctaState& state, TurnAnimOcta& anim,
                      int& depth, sf::Clock& scrambleClock, OctaMove& pending, bool& showDevNames) {
    const auto key = kp.code;
    if (key == sf::Keyboard::Key::Escape) {
        window.close();
        return;
    }
    using Sc = sf::Keyboard::Scan;
    const bool zeroByCode = (key == sf::Keyboard::Key::Num0 || key == sf::Keyboard::Key::Numpad0);
    const bool zeroByScan = (kp.scancode == Sc::Num0 || kp.scancode == Sc::Numpad0);
    if (zeroByCode || zeroByScan) {
        showDevNames = !showDevNames;
        return;
    }
    const bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    const int vdir = shift ? -1 : 1;
    handlePuzzleKeys(key, depth, vdir, state, anim, scrambleClock, pending);
}

void processFrameEvents(sf::RenderWindow& window, Renderer& renderer, OctaState& state, TurnAnimOcta& anim, int& depth,
                        float& yawDeg, float& pitchDeg, LeftDragKind& leftDrag, sf::Vector2i& dragPos,
                        sf::Vector2i& windowDragGrab, sf::Clock& scrambleClock, OctaMove& pending,
                        bool& showDevNames) {
    while (const std::optional<sf::Event> ev = window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* rs = ev->getIf<sf::Event::Resized>()) {
            handleResizeEvent(*rs, window, renderer);
        }
        if (const auto* mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
            handleMousePressed(*mb, window, leftDrag, dragPos, windowDragGrab);
        }
        if (const auto* mr = ev->getIf<sf::Event::MouseButtonReleased>()) {
            handleMouseReleased(*mr, leftDrag);
        }
        if (const auto* mm = ev->getIf<sf::Event::MouseMoved>()) {
            handleMouseMoved(*mm, window, leftDrag, dragPos, windowDragGrab, yawDeg, pitchDeg);
        }
        if (const auto* wheel = ev->getIf<sf::Event::MouseWheelScrolled>()) {
            handleWheelScrolled(*wheel, renderer);
        }
        if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
            handleKeyPressed(*kp, window, state, anim, depth, scrambleClock, pending, showDevNames);
        }
    }
}

void renderFrame(sf::RenderWindow& window, Renderer& renderer, float yawDeg, float pitchDeg, float rollDeg,
                 const OctaMesh& mesh, OctaState& state, TurnAnimOcta& anim, std::optional<sf::Text>& hud,
                 std::optional<sf::Text>& devHud, bool showDevNames) {
    renderer.resize(static_cast<int>(window.getSize().x), static_cast<int>(window.getSize().y));
    static_cast<void>(window.setActive(true));
    renderer.beginScene(yawDeg, pitchDeg, rollDeg);
    renderer.drawOcta(mesh, state.colors(), kMeshViewScale, anim.active ? &anim : nullptr, yawDeg, pitchDeg, rollDeg);
    if (showDevNames) {
        for (int fi = 0; fi < kOctaFaces; ++fi) {
            const auto face = static_cast<OctaFace>(fi);
            if (renderer.faceFacesCamera(face, yawDeg, pitchDeg, rollDeg)) {
                renderer.drawOctaWireframe(mesh, kMeshViewScale, anim.active ? &anim : nullptr, face);
            }
        }
    }
    if (hud.has_value()) {
        drawHudOverlay(window, *hud);
    }
    if (showDevNames && devHud.has_value()) {
        drawDevNamesOnOcta(window, renderer, mesh, yawDeg, pitchDeg, rollDeg, kMeshViewScale, anim, *devHud);
    }
    window.display();
}

void runApplication() {
    const sf::ContextSettings settings = makeGlContextSettings();

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(kWindowW, kWindowH)), "Octahedron", sf::Style::None,
                            sf::State::Windowed, settings);
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);
    static_cast<void>(window.setActive(true));
    applySquareWindowShape(window);

    Renderer renderer;
    renderer.initialize();

    const OctaMesh mesh = buildOctaMesh(1.0f);
    OctaState state(mesh);
    state.scramble(36, static_cast<unsigned>(std::time(nullptr)));

    TurnAnimOcta anim{};
    OctaMove pending{};

    int depth = 1;
    float yawDeg = 35.f;
    float pitchDeg = 22.f;
    const float rollDeg = 45.f;
    LeftDragKind leftDrag = LeftDragKind::None;
    sf::Vector2i dragPos{};
    sf::Vector2i windowDragGrab{};

    sf::Clock frameClock;
    sf::Clock scrambleClock;
    sf::Font font;
    std::optional<sf::Text> hud;
    std::optional<sf::Text> devHud;
    static_cast<void>(tryLoadHudFont(font, hud, devHud));
    bool showDevNames = false;

    while (window.isOpen()) {
        const float dt = frameClock.restart().asSeconds();

        processFrameEvents(window, renderer, state, anim, depth, yawDeg, pitchDeg, leftDrag, dragPos, windowDragGrab,
                           scrambleClock, pending, showDevNames);

        advanceTurnAnimation(dt, anim, state, pending);

        renderFrame(window, renderer, yawDeg, pitchDeg, rollDeg, mesh, state, anim, hud, devHud, showDevNames);
    }
}

} // namespace

int main() {
    runApplication();
    return 0;
}

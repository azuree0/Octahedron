// GL 2 fixed-function: orbit, mesh, lighting.

#pragma once

#include <SFML/OpenGL.hpp>

#include "octa_mesh.h"
#include "octa_state.h"
#include "math3.h"

#include <array>

inline constexpr float kOctaTurnDeg = 120.f;

struct TurnAnimOcta {
    bool active = false;
    OctaFace face = OctaFace::Face1;
    int depth = 1;
    int dir = 1;
    float currentDeg = 0.f;
};

class Renderer {
public:
    Renderer();

    void initialize();
    void resize(int width, int height);
    void handleMouseWheel(int delta);
    void beginScene(float yawDeg, float pitchDeg, float rollDeg);
    void drawOcta(const OctaMesh& mesh, const std::array<int, kStickerCount>& colors, float scale,
                  const TurnAnimOcta* anim, float yawDeg, float pitchDeg, float rollDeg);

    void drawOctaWireframe(const OctaMesh& mesh, float scale, const TurnAnimOcta* anim, OctaFace onlyFace) const;

    OctaFace dominantFacingFace(float yawDeg, float pitchDeg, float rollDeg) const;
    bool faceFacesCamera(OctaFace face, float yawDeg, float pitchDeg, float rollDeg) const;

    bool projectMeshAnchorToWindow(float meshScale, float yawDeg, float pitchDeg, float rollDeg, Vec3 meshLocal,
                                   const TurnAnimOcta* anim, int viewportW, int viewportH, double& outPx, double& outPy,
                                   bool& outVisible, int stickerSlot = -1) const;

private:
    void drawStars();

    float cameraDistance_;
    static constexpr float kFovYDeg = 45.f;
    static constexpr float kCameraDistMin = 3.0f;
    static constexpr float kCameraDistMax = 16.f;
};

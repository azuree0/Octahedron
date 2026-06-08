// Aesthetic: translucent green-blue glass (teal through cerulean); black starfield backdrop unchanged.
//
// Geometry: camera/orbit matrices, mesh transforms, slab twist, projections, wireframe paths.
// Lighting: fixed-function lights, sticker materials (palette + emission), unlit star shell.

#include "renderer.h"

#include <GL/glu.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

void hsvToRgb(float hDeg, float s, float v, float rgb[3]) {
    hDeg = std::fmod(hDeg, 360.f);
    if (hDeg < 0.f) {
        hDeg += 360.f;
    }
    const float c = v * s;
    const float x = c * (1.f - std::fabs(std::fmod(hDeg / 60.f, 2.f) - 1.f));
    const float m = v - c;
    float rp = 0.f;
    float gp = 0.f;
    float bp = 0.f;
    const int sector = static_cast<int>(hDeg / 60.f);
    switch (sector) {
    case 0:
        rp = c;
        gp = x;
        break;
    case 1:
        rp = x;
        gp = c;
        break;
    case 2:
        gp = c;
        bp = x;
        break;
    case 3:
        gp = x;
        bp = c;
        break;
    case 4:
        rp = x;
        bp = c;
        break;
    default:
        rp = c;
        bp = x;
        break;
    }
    rgb[0] = std::clamp(rp + m, 0.f, 1.f);
    rgb[1] = std::clamp(gp + m, 0.f, 1.f);
    rgb[2] = std::clamp(bp + m, 0.f, 1.f);
}

// Aesthetic: eight face hues spaced wide (yellow-green through indigo-violet).
void diffuseGreenBlue(int face, float out[4]) {
    const int f = face % 8;
    static const struct {
        float hDeg;
        float s;
        float v;
    } kGreenBlueHues[8] = {
        {112.f, 0.52f, 0.76f}, // chartreuse green
        {135.f, 0.50f, 0.72f}, // spring green
        {158.f, 0.48f, 0.70f}, // jade
        {180.f, 0.52f, 0.68f}, // turquoise
        {202.f, 0.46f, 0.70f}, // cyan
        {222.f, 0.50f, 0.66f}, // azure
        {245.f, 0.48f, 0.64f}, // cobalt
        {268.f, 0.42f, 0.68f}, // indigo violet
    };
    const auto& b = kGreenBlueHues[f];
    hsvToRgb(b.hDeg, b.s, b.v, out);
    out[3] = 1.0f;
}

float degToRad(float d) {
    return d * static_cast<float>(M_PI / 180.0);
}

void setupFramebufferAndShadeModel() {
    glDisable(GL_CULL_FACE);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Aesthetic: pure black backdrop so the unlit star shell reads clearly.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glShadeModel(GL_SMOOTH);

    const float globalAmb[4] = {0.11f, 0.12f, 0.13f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void setupKeyLight() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    const float keyAmb[4] = {0.08f, 0.09f, 0.10f, 1.0f};
    // Lighting: main key brightness (diffuse/specular only; ambient unchanged for shadow depth).
    const float keyDiff[4] = {3.00f, 2.40f, 2.60f, 1.0f};
    const float keySpec[4] = {3.00f, 2.55f, 2.75f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, keyAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, keyDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, keySpec);

    const float fillAmb[4] = {0.05f, 0.07f, 0.08f, 1.0f};
    const float fillDiff[4] = {0.22f, 0.34f, 0.40f, 1.0f};
    const float fillSpec[4] = {0.09f, 0.15f, 0.17f, 1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT, fillAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fillDiff);
    glLightfv(GL_LIGHT1, GL_SPECULAR, fillSpec);

    const float z[4] = {0.f, 0.f, 0.f, 1.0f};
    for (GLenum li = GL_LIGHT2; li <= GL_LIGHT6; ++li) {
        glEnable(li);
        glLightfv(li, GL_AMBIENT, z);
        glLightfv(li, GL_DIFFUSE, z);
        glLightfv(li, GL_SPECULAR, z);
    }
}

void emitStarPointsOnSphere(int count, float pointSize, float cr, float cg, float cb) {
    glPointSize(pointSize);
    glColor3f(cr, cg, cb);
    for (int i = 0; i < count; ++i) {
        const float theta = static_cast<float>(std::rand() % 628) / 100.0f;
        const float phi = static_cast<float>(std::rand() % 314) / 100.0f;
        constexpr float radius = 50.0f;
        const float x = radius * std::sin(phi) * std::cos(theta);
        const float y = radius * std::sin(phi) * std::sin(theta);
        const float z = radius * std::cos(phi);
        glVertex3f(x, y, z);
    }
}

Vec3 rotateAnim(Vec3 v, const Vec3& axis, float angRad) {
    if (std::fabs(angRad) < 1e-6f) {
        return v;
    }
    return rotateAroundAxis(v, normalize(axis), angRad);
}

Vec3 transformMeshPointForTurnByPosition(Vec3 p, const TurnAnimOcta* anim, const OctaMesh& mesh) {
    if (anim == nullptr || !anim->active) {
        return p;
    }
    int bestSlot = 0;
    float bestD2 = 1e30f;
    for (int i = 0; i < kStickerCount; ++i) {
        const Vec3 d = mesh.centroids[static_cast<std::size_t>(i)] - p;
        const float d2 = dot(d, d);
        if (d2 < bestD2) {
            bestD2 = d2;
            bestSlot = i;
        }
    }
    if (!OctaState::inSlab(anim->face, anim->depth, bestSlot, mesh)) {
        return p;
    }
    const Vec3 axis = OctaState::faceOutwardNormal(anim->face);
    const float angRad = static_cast<float>(anim->dir) * degToRad(anim->currentDeg);
    return rotateAnim(p, axis, angRad);
}

Vec3 outwardNormalMesh(OctaFace f) {
    return OctaState::faceOutwardNormal(f);
}

Vec3 orbitRotateDirection(Vec3 v, float yawDeg, float pitchDeg, float rollDeg) {
    Vec3 v1 = rotateAroundAxis(v, {0.f, 0.f, 1.f}, degToRad(rollDeg));
    Vec3 v2 = rotateAroundAxis(v1, {0.f, 1.f, 0.f}, degToRad(yawDeg));
    Vec3 v3 = rotateAroundAxis(v2, {1.f, 0.f, 0.f}, degToRad(pitchDeg));
    return v3;
}

} // namespace

Renderer::Renderer() : cameraDistance_(3.2f) {}

void Renderer::initialize() {
    setupFramebufferAndShadeModel();
    setupKeyLight();
}

void Renderer::handleMouseWheel(int delta) {
    cameraDistance_ += static_cast<float>(delta) * 0.25f;
    cameraDistance_ = std::max(kCameraDistMin, std::min(kCameraDistMax, cameraDistance_));
}

void Renderer::resize(int width, int height) {
    if (height <= 0) {
        height = 1;
    }
    glViewport(0, 0, width, height);
}

void Renderer::drawStars() {
    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glBegin(GL_POINTS);
    std::srand(42);
    // Lighting: unlit white star shell on black (deterministic srand(42) from caller).
    emitStarPointsOnSphere(150, 2.0f, 1.0f, 1.0f, 1.0f);
    emitStarPointsOnSphere(15, 3.0f, 1.0f, 1.0f, 0.9f);
    glEnd();

    glEnable(GL_LIGHTING);
}

void Renderer::beginScene(float yawDeg, float pitchDeg, float rollDeg) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const double aspect =
        static_cast<double>(viewport[2]) / static_cast<double>(viewport[3] > 0 ? viewport[3] : 1);
    gluPerspective(static_cast<double>(kFovYDeg), aspect, 0.1, 250.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -cameraDistance_);
    glRotatef(pitchDeg, 1.0f, 0.0f, 0.0f);
    glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
    glRotatef(rollDeg, 0.0f, 0.0f, 1.0f);

    // Lighting: key light aligned with the single face most toward the camera this frame.
    const OctaFace keyFace = dominantFacingFace(yawDeg, pitchDeg, rollDeg);
    const Vec3 keyNormal = outwardNormalMesh(keyFace);
    const float lightDir[4] = {keyNormal.x, keyNormal.y, keyNormal.z, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightDir);

    const float fillDir[4] = {0.4f, -0.55f, -0.72f, 0.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, fillDir);

    const float axisAmb[4] = {0.04f, 0.06f, 0.07f, 1.0f};
    const float axisDiff[4] = {0.30f, 0.46f, 0.54f, 1.0f};
    const float axisSpec[4] = {0.09f, 0.14f, 0.16f, 1.0f};
    const GLenum axisLights[4] = {GL_LIGHT3, GL_LIGHT4, GL_LIGHT5, GL_LIGHT6};
    const float axisObj[4][4] = {
        {1.f, 0.f, 0.f, 0.f},
        {-1.f, 0.f, 0.f, 0.f},
        {0.f, 1.f, 0.f, 0.f},
        {0.f, -1.f, 0.f, 0.f},
    };
    for (int i = 0; i < 4; ++i) {
        glLightfv(axisLights[i], GL_AMBIENT, axisAmb);
        glLightfv(axisLights[i], GL_DIFFUSE, axisDiff);
        glLightfv(axisLights[i], GL_SPECULAR, axisSpec);
        glLightfv(axisLights[i], GL_POSITION, axisObj[i]);
    }

    glPushMatrix();
    glLoadIdentity();
    const float headAmb2[4] = {0.f, 0.f, 0.f, 1.0f};
    const float headDiff2[4] = {0.78f, 0.98f, 1.02f, 1.0f};
    const float headSpec2[4] = {0.24f, 0.40f, 0.42f, 1.0f};
    glLightfv(GL_LIGHT2, GL_AMBIENT, headAmb2);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, headDiff2);
    glLightfv(GL_LIGHT2, GL_SPECULAR, headSpec2);
    const float headPos2[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT2, GL_POSITION, headPos2);
    glPopMatrix();

    drawStars();
}

void Renderer::drawOcta(const OctaMesh& mesh, const std::array<int, kStickerCount>& colors, float scale,
                         const TurnAnimOcta* anim, float yawDeg, float pitchDeg, float rollDeg) {
    thread_local std::vector<char> mask;
    mask.assign(static_cast<std::size_t>(kStickerCount), 0);
    const bool useAnim = anim != nullptr && anim->active;
    Vec3 axis{0.f, 1.f, 0.f};
    float angRad = 0.f;
    if (useAnim) {
        axis = OctaState::faceOutwardNormal(anim->face);
        angRad = static_cast<float>(anim->dir) * degToRad(anim->currentDeg);
        std::vector<int> moving;
        OctaState::stickersInSlab(anim->face, anim->depth, mesh, moving);
        for (int id : moving) {
            mask[static_cast<std::size_t>(id)] = 1;
        }
    }

    // Aesthetic: glass specular and alpha (stars show through the mesh).
    constexpr float kGlassAlpha = 0.50f;
    const OctaFace keyFace = dominantFacingFace(yawDeg, pitchDeg, rollDeg);

    const auto emitSticker = [&](int s, bool rotateSlab) {
        const int pal = colors[static_cast<std::size_t>(s)];
        float diff[4];
        diffuseGreenBlue(pal, diff);

        const int faceIdx = s / kTrisPerFace;
        const OctaFace mf = static_cast<OctaFace>(faceIdx);
        const Vec3 nEye = orbitRotateDirection(outwardNormalMesh(mf), yawDeg, pitchDeg, rollDeg);
        const float nz = nEye.z;
        const bool isKeyFace = (mf == keyFace);
        const bool frontalShine = isKeyFace && (nz >= 0.12f);

        constexpr float kAmbFloor = 0.06f;
        float ambK = isKeyFace ? 0.36f : 0.20f;
        float emitK = isKeyFace ? 0.10f : 0.03f;
        float diffR = diff[0];
        float diffG = diff[1];
        float diffB = diff[2];
        if (!isKeyFace) {
            constexpr float kNonKeyDim = 0.58f;
            diffR *= kNonKeyDim;
            diffG *= kNonKeyDim;
            diffB *= kNonKeyDim;
        }
        if (frontalShine) {
            const float t = std::clamp((nz - 0.12f) / 0.78f, 0.f, 1.f);
            ambK = std::max(ambK, 0.62f + 0.45f * t);
            const float diffBoost = 1.0f + 0.72f * t;
            diffR = std::min(1.f, diffR * diffBoost);
            diffG = std::min(1.f, diffG * diffBoost);
            diffB = std::min(1.f, diffB * diffBoost);
            emitK = std::max(emitK, 0.08f + 0.14f * t);
        }

        const float specMat[4] = {frontalShine ? 0.72f : 0.40f, frontalShine ? 0.88f : 0.52f,
                                  frontalShine ? 0.92f : 0.56f, 1.0f};

        const float ambMat[4] = {std::min(1.f, kAmbFloor + diffR * ambK),
                                 std::min(1.f, kAmbFloor + diffG * ambK),
                                 std::min(1.f, kAmbFloor + diffB * ambK), kGlassAlpha};
        const float emitMat[4] = {std::max(0.02f, diffR * emitK), std::max(0.02f, diffG * emitK),
                                   std::max(0.02f, diffB * emitK), kGlassAlpha * 0.5f};
        const float diffMat[4] = {diffR, diffG, diffB, kGlassAlpha};

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambMat);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffMat);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specMat);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emitMat);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, frontalShine ? 96.0f : (isKeyFace ? 72.0f : 48.0f));

        const MeshTriangle& tri = mesh.triangles[static_cast<std::size_t>(s)];
        Vec3 n = tri.normal;
        if (rotateSlab) {
            n = rotateAnim(n, axis, angRad);
        }
        glNormal3f(n.x, n.y, n.z);
        for (int k = 0; k < 3; ++k) {
            Vec3 p = tri.pos[static_cast<std::size_t>(k)];
            if (rotateSlab) {
                p = rotateAnim(p, axis, angRad);
            }
            glVertex3f(p.x, p.y, p.z);
        }
    };

    const auto drawMeshBody = [&]() {
        if (useAnim) {
            glBegin(GL_TRIANGLES);
            for (int s = 0; s < kStickerCount; ++s) {
                if (mask[static_cast<std::size_t>(s)] != 0) {
                    continue;
                }
                emitSticker(s, false);
            }
            glEnd();

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.2f, -2.5f);
            glBegin(GL_TRIANGLES);
            for (int s = 0; s < kStickerCount; ++s) {
                if (mask[static_cast<std::size_t>(s)] == 0) {
                    continue;
                }
                emitSticker(s, true);
            }
            glEnd();
            glDisable(GL_POLYGON_OFFSET_FILL);
        } else {
            glBegin(GL_TRIANGLES);
            for (int s = 0; s < kStickerCount; ++s) {
                emitSticker(s, false);
            }
            glEnd();
        }
    };

    glPushMatrix();
    glScalef(scale, scale, scale);

    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    drawMeshBody();
    glCullFace(GL_BACK);
    drawMeshBody();
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);

    glPopMatrix();
}

OctaFace Renderer::dominantFacingFace(float yawDeg, float pitchDeg, float rollDeg) const {
    OctaFace best = OctaFace::Face1;
    float bestZ = -1e10f;
    for (int i = 0; i < kOctaFaces; ++i) {
        const auto f = static_cast<OctaFace>(i);
        const Vec3 nEye = orbitRotateDirection(outwardNormalMesh(f), yawDeg, pitchDeg, rollDeg);
        if (nEye.z > bestZ) {
            bestZ = nEye.z;
            best = f;
        }
    }
    return best;
}

bool Renderer::faceFacesCamera(OctaFace face, float yawDeg, float pitchDeg, float rollDeg) const {
    const Vec3 nEye = orbitRotateDirection(outwardNormalMesh(face), yawDeg, pitchDeg, rollDeg);
    constexpr float kGrazingNZSlack = 0.02f;
    return nEye.z >= -kGrazingNZSlack;
}

void Renderer::drawOctaWireframe(const OctaMesh& mesh, float scale, const TurnAnimOcta* anim,
                                 OctaFace onlyFace) const {
    glDisable(GL_LIGHTING);
    // Aesthetic: wireframe overlay in very light sky blue.
    glColor3f(0.78f, 0.92f, 1.0f);
    glLineWidth(1.5f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glPushMatrix();
    glScalef(scale, scale, scale);
    glBegin(GL_LINES);

    const int fi = static_cast<int>(onlyFace);
    for (int t = 0; t < kTrisPerFace; ++t) {
        const int stickerSlot = fi * kTrisPerFace + t;
        const MeshTriangle& tri = mesh.triangles[static_cast<std::size_t>(stickerSlot)];
        for (int e = 0; e < 3; ++e) {
            const int k0 = e;
            const int k1 = (e + 1) % 3;
            Vec3 p0 = tri.pos[static_cast<std::size_t>(k0)];
            Vec3 p1 = tri.pos[static_cast<std::size_t>(k1)];
            if (anim != nullptr && anim->active) {
                p0 = transformMeshPointForTurnByPosition(p0, anim, mesh);
                p1 = transformMeshPointForTurnByPosition(p1, anim, mesh);
            }
            glVertex3f(p0.x, p0.y, p0.z);
            glVertex3f(p1.x, p1.y, p1.z);
        }
    }

    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

bool Renderer::projectMeshAnchorToWindow(float meshScale, float yawDeg, float pitchDeg, float rollDeg, Vec3 meshLocal,
                                         const TurnAnimOcta* anim, int viewportW, int viewportH, double& outPx,
                                         double& outPy, bool& outVisible, int stickerSlot) const {
    if (viewportW <= 0 || viewportH <= 0) {
        outVisible = false;
        return false;
    }

    OctaMesh mesh = buildOctaMesh(1.f);
    Vec3 p = meshLocal;
    if (anim != nullptr && anim->active && stickerSlot >= 0) {
        if (OctaState::inSlab(anim->face, anim->depth, stickerSlot, mesh)) {
            const Vec3 axis = OctaState::faceOutwardNormal(anim->face);
            const float angRad = static_cast<float>(anim->dir) * degToRad(anim->currentDeg);
            p = rotateAnim(p, axis, angRad);
        }
    }

    GLdouble proj[16]{};
    GLdouble mv[16]{};
    GLint vp[4] = {0, 0, viewportW, viewportH};
    const double aspect =
        static_cast<double>(viewportW) / static_cast<double>(viewportH > 0 ? viewportH : 1);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(static_cast<double>(kFovYDeg), aspect, 0.1, 250.0);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -cameraDistance_);
    glRotatef(pitchDeg, 1.0f, 0.0f, 0.0f);
    glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
    glRotatef(rollDeg, 0.0f, 0.0f, 1.0f);
    glScalef(meshScale, meshScale, meshScale);
    glGetDoublev(GL_MODELVIEW_MATRIX, mv);

    GLdouble winX = 0.0;
    GLdouble winY = 0.0;
    GLdouble winZ = 0.0;
    const GLboolean ok = gluProject(static_cast<GLdouble>(p.x), static_cast<GLdouble>(p.y), static_cast<GLdouble>(p.z),
                                     mv, proj, vp, &winX, &winY, &winZ);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    outVisible = (ok == GL_TRUE) && (winZ >= 0.0 && winZ <= 1.0);
    outPx = winX;
    outPy = static_cast<double>(viewportH) - winY;
    return ok == GL_TRUE;
}

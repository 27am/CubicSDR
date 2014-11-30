#include "WaterfallContext.h"
#include "WaterfallCanvas.h"
#include "CubicSDR.h"

WaterfallContext::WaterfallContext(WaterfallCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glGenTextures(1, &waterfall);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, waterfall);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    grad.addColor(GradientColor(0, 0, 0));
    grad.addColor(GradientColor(0, 0, 1.0));
    grad.addColor(GradientColor(0, 1.0, 0));
    grad.addColor(GradientColor(1.0, 1.0, 0));
    grad.addColor(GradientColor(1.0, 0.2, 0.0));

    grad.generate(256);

    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, &(grad.getRed())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, &(grad.getGreen())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, &(grad.getBlue())[0]);
}

void WaterfallContext::BeginDraw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void WaterfallContext::Draw(std::vector<float> &points) {

    if (points.size()) {
        memmove(waterfall_tex + FFT_SIZE, waterfall_tex, (NUM_WATERFALL_LINES - 1) * FFT_SIZE);

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            float v = points[i * 2 + 1];

            float wv = v;
            if (wv < 0.0)
                wv = 0.0;
            if (wv > 1.0)
                wv = 1.0;
            waterfall_tex[i] = (unsigned char) floor(wv * 255.0);
        }
    }

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, waterfall);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FFT_SIZE, NUM_WATERFALL_LINES, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, (GLvoid *) waterfall_tex);

    glColor3f(1.0, 1.0, 1.0);

    glBindTexture(GL_TEXTURE_2D, waterfall);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-1.0, -1.0, 0.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.0, -1.0, 0.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(1.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.0, 1.0, 0.0);
    glEnd();

}

void WaterfallContext::DrawDemod(DemodulatorInstance *demod, float r, float g, float b) {
    if (!demod) {
        return;
    }

    float uxPos = (float) (demod->getParams().frequency - (wxGetApp().getFrequency() - SRATE / 2)) / (float) SRATE;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
    glColor4f(r, g, b, 0.6);

    glBegin(GL_LINES);
    glVertex3f((uxPos - 0.5) * 2.0, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0, -1.0, 0.0);

    float ofs = ((float) demod->getParams().bandwidth) / (float) SRATE;

    glVertex3f((uxPos - 0.5) * 2.0 - ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 - ofs, -1.0, 0.0);

    glVertex3f((uxPos - 0.5) * 2.0 + ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 + ofs, -1.0, 0.0);

    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
    glColor4f(r, g, b, 0.2);
    glBegin(GL_QUADS);
    glVertex3f((uxPos - 0.5) * 2.0 - ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 - ofs, -1.0, 0.0);

    glVertex3f((uxPos - 0.5) * 2.0 + ofs, -1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 + ofs, 1.0, 0.0);
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

}

void WaterfallContext::DrawFreqSelector(float uxPos, float r, float g, float b) {
    DemodulatorInstance *demod = wxGetApp().getDemodTest();

    if (!demod) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
    glColor4f(r, g, b, 0.6);

    glBegin(GL_LINES);
    glVertex3f((uxPos - 0.5) * 2.0, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0, -1.0, 0.0);

    float ofs = ((float) demod->getParams().bandwidth) / (float) SRATE;

    glVertex3f((uxPos - 0.5) * 2.0 - ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 - ofs, -1.0, 0.0);

    glVertex3f((uxPos - 0.5) * 2.0 + ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 + ofs, -1.0, 0.0);

    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

}

void WaterfallContext::EndDraw() {
    glFlush();

    CheckGLError();
}

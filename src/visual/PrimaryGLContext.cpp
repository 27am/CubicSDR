#include "PrimaryGLContext.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "CubicSDRDefs.h"
#include "AppFrame.h"
#include <algorithm>

GLFont PrimaryGLContext::fonts[GLFONT_MAX];

wxString PrimaryGLContext::glGetwxString(GLenum name) {
    const GLubyte *v = glGetString(name);
    if (v == 0) {
        // The error is not important. It is GL_INVALID_ENUM.
        // We just want to clear the error stack.
        glGetError();

        return wxString();
    }

    return wxString((const char*) v);
}

void PrimaryGLContext::CheckGLError() {
    GLenum errLast = GL_NO_ERROR;

    for (;;) {
        GLenum err = glGetError();
        if (err == GL_NO_ERROR)
            return;

        if (err == errLast) {
            wxLogError
            (wxT("OpenGL error state couldn't be reset."));
            return;
        }

        errLast = err;

        wxLogError
        (wxT("OpenGL error %d"), err);
    }
}

PrimaryGLContext::PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext) :
        wxGLContext(canvas, sharedContext) {
    SetCurrent(*canvas);

    CheckGLError();
}

GLFont &PrimaryGLContext::getFont(GLFontSize esize) {
    if (!fonts[esize].isLoaded()) {

        std::string fontName;
        switch (esize) {
        case GLFONT_SIZE12: fontName = "vera_sans_mono12.fnt";
        break;
        case GLFONT_SIZE16: fontName = "vera_sans_mono16.fnt";
        break;
        case GLFONT_SIZE18: fontName = "vera_sans_mono18.fnt";
        break;
        case GLFONT_SIZE24: fontName = "vera_sans_mono24.fnt";
        break;
        case GLFONT_SIZE32: fontName = "vera_sans_mono32.fnt";
        break;
        case GLFONT_SIZE48: fontName = "vera_sans_mono48.fnt";
        break;
        }

        fonts[esize].loadFont(fontName);
    }

    return fonts[esize];
}

void PrimaryGLContext::DrawDemodInfo(DemodulatorInstance *demod, float r, float g, float b) {
    if (!demod) {
        return;
    }

    float uxPos = (float) (demod->getParams().frequency - (wxGetApp().getFrequency() - SRATE / 2)) / (float) SRATE;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
    glColor4f(r, g, b, 0.6);

    float ofs = ((float) demod->getParams().bandwidth) / (float) SRATE;

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

void PrimaryGLContext::DrawDemod(DemodulatorInstance *demod, float r, float g, float b) {
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

void PrimaryGLContext::DrawFreqSelector(float uxPos, float r, float g, float b) {
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

void PrimaryGLContext::BeginDraw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void PrimaryGLContext::EndDraw() {
    glFlush();

    CheckGLError();
}


#include "WaterfallContext.h"
#include "WaterfallCanvas.h"
#include "CubicSDR.h"

WaterfallContext::WaterfallContext(WaterfallCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext), waterfall(0), waterfall_tex(NULL), waterfall_lines(0), fft_size(0), theme(COLOR_THEME_DEFAULT) {
    Gradient *grad = new Gradient();
    grad->addColor(GradientColor(0, 0, 0));
    grad->addColor(GradientColor(0, 0, 1.0));
    grad->addColor(GradientColor(0, 1.0, 0));
    grad->addColor(GradientColor(1.0, 1.0, 0));
    grad->addColor(GradientColor(1.0, 0.2, 0.0));
    grad->generate(256);
    gradients[COLOR_THEME_DEFAULT] = grad;

    grad = new Gradient();
    grad->addColor(GradientColor(0, 0, 0));
    grad->addColor(GradientColor(0.0, 0, 0.5));
    grad->addColor(GradientColor(0.0, 0.0, 1.0));
    grad->addColor(GradientColor(65.0 / 255.0, 161.0 / 255.0, 1.0));
    grad->addColor(GradientColor(1.0, 1.0, 1.0));
    grad->addColor(GradientColor(1.0, 1.0, 1.0));
    grad->addColor(GradientColor(1.0, 1.0, 0.5));
    grad->addColor(GradientColor(1.0, 1.0, 0.0));
    grad->addColor(GradientColor(1.0, 0.5, 0.0));
    grad->addColor(GradientColor(1.0, 0.25, 0.0));
    grad->addColor(GradientColor(0.5, 0.1, 0.0));
    grad->generate(256);
    gradients[COLOR_THEME_SHARP] = grad;

    grad = new Gradient();
    grad->addColor(GradientColor(0, 0, 0));
    grad->addColor(GradientColor(0.75, 0.75, 0.75));
    grad->addColor(GradientColor(1.0, 1.0, 1.0));
    grad->generate(256);
    gradients[COLOR_THEME_BW] = grad;

    grad = new Gradient();
    grad->addColor(GradientColor(0, 0, 0.5));
    grad->addColor(GradientColor(25.0/255.0, 154.0/255.0, 0.0));
    grad->addColor(GradientColor(201.0/255.0, 115.0/255.0, 0.0));
    grad->addColor(GradientColor(1.0, 40.0/255.0, 40.0/255.0));
    grad->addColor(GradientColor(1.0, 1.0, 1.0));
    grad->generate(256);
    gradients[COLOR_THEME_RAD] = grad;


    grad = new Gradient();
    grad->addColor(GradientColor(0, 0, 0));
    grad->addColor(GradientColor(55.0/255.0, 40.0/255.0, 55.0/255.0));
    grad->addColor(GradientColor(60.0/255.0, 60.0/255.0, 90.0/255.0));
    grad->addColor(GradientColor(0.0/255.0, 255.0/255.0, 255.0/255.0));
    grad->addColor(GradientColor(10.0/255.0, 255.0/255.0, 85.0/255.0));
    grad->addColor(GradientColor(255.0/255.0, 255.0/255.0, 75.0/255.0));
    grad->addColor(GradientColor(255.0/255.0, 0.0/255.0, 0.0/255.0));
    grad->addColor(GradientColor(255.0/255.0, 255.0/255.0, 255.0/255.0));
    grad->generate(256);
    gradients[COLOR_THEME_TOUCH] = grad;


    grad = new Gradient();
    grad->addColor(GradientColor(5.0/255.0, 5.0/255.0, 60.0/255.0));
    grad->addColor(GradientColor(5.0/255.0, 20.0/255.0, 120.0/255.0));
    grad->addColor(GradientColor(50.0/255.0, 100.0/255.0, 200.0/255.0));
    grad->addColor(GradientColor(75.0/255.0, 190.0/255.0, 100.0/255.0));
    grad->addColor(GradientColor(240.0/255.0, 55.0/255.0, 5.0/255.0));
    grad->addColor(GradientColor(255.0/255.0, 55.0/255.0, 100.0/255.0));
    grad->addColor(GradientColor(255.0/255.0, 235.0/255.0, 100.0/255.0));
    grad->addColor(GradientColor(250.0/255.0, 250.0/255.0, 250.0/255.0));
    grad->generate(256);
    gradients[COLOR_THEME_HD] = grad;




    grad = new Gradient();
    grad->addColor(GradientColor(5.0/255.0, 45.0/255.0, 10.0/255.0));
    grad->addColor(GradientColor(30.0/255.0, 150.0/255.0, 40.0/255.0));
    grad->addColor(GradientColor(40.0/255.0, 240.0/255.0, 60.0/255.0));
    grad->addColor(GradientColor(250.0/255.0, 250.0/255.0, 250.0/255.0));
    grad->generate(256);
    gradients[COLOR_THEME_RADAR] = grad;
}

void WaterfallContext::Setup(int fft_size_in, int num_waterfall_lines_in) {
    if (waterfall) {
        glDeleteTextures(1, &waterfall);
        waterfall = 0;
    }
    if (waterfall_tex) {
        delete waterfall_tex;
    }

    waterfall_lines = num_waterfall_lines_in;
    fft_size = fft_size_in;

    waterfall_tex = new unsigned char[fft_size * waterfall_lines];
    memset(waterfall_tex, 0, fft_size * waterfall_lines);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

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

    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, &(gradients[theme]->getRed())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, &(gradients[theme]->getGreen())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, &(gradients[theme]->getBlue())[0]);

}

void WaterfallContext::setTheme(int theme_id) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, waterfall);

    if (theme >= COLOR_THEME_MAX) {
        theme = COLOR_THEME_MAX - 1;
    }
    if (theme < 0) {
        theme = 0;
    }
    theme = theme_id;

    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, &(gradients[theme]->getRed())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, &(gradients[theme]->getGreen())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, &(gradients[theme]->getBlue())[0]);
}

void WaterfallContext::Draw(std::vector<float> &points) {

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    if (points.size()) {
        memmove(waterfall_tex + fft_size, waterfall_tex, (waterfall_lines - 1) * fft_size);

        for (int i = 0, iMax = fft_size; i < iMax; i++) {
            float v = points[i * 2 + 1];

            float wv = v;
            if (wv < 0.0)
                wv = 0.0;
            if (wv > 0.99)
                wv = 0.99;
            waterfall_tex[i] = (unsigned char) floor(wv * 255.0);
        }
    }

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, waterfall);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fft_size, waterfall_lines, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, (GLvoid *) waterfall_tex);

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

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

}

int WaterfallContext::getTheme() {
    return theme;
}

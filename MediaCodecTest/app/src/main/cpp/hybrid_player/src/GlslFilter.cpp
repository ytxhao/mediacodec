//
// Created by Administrator on 2017/4/7.
//

#define LOG_NDEBUG 0
#define TAG "YTX-GlslFilter-JNI"
#include <ALog-priv.h>
#include <GlslFilter.h>
#include "GlslFilter.h"

static const char gVertexShader[]=
        "attribute vec4 a_position;\n"
        "attribute vec2 a_texcoord;\n"
        "varying vec2 textureCoordinate;\n"
                "void main() {\n"
                "  gl_Position = a_position;\n"
                "  textureCoordinate = a_texcoord;\n"
                "}\n";

static const char gFragmentShader[]=
        "#extension GL_OES_EGL_image_external : require\n"
        "precision mediump float;\n"
        "uniform samplerExternalOES inputImageTexture;\n"
        "varying vec2 textureCoordinate;\n"
                "void main(){\n"
                "  gl_FragColor = texture2D(inputImageTexture, textureCoordinate);\n"
                "}\n";




GlslFilter::GlslFilter() {
    isInitialed = false;
    frameBufferObjectId = 0;
    //textureOut = 1025;
}

GlslFilter::~GlslFilter() {

}


const char * GlslFilter::getVertexShaderString() {


    return gVertexShader;
}

const char * GlslFilter::getFragmentShaderString() {

    return gFragmentShader;

}
void GlslFilter::initial() {

    if(isInitialed){
        return;
    }

    isInitialed = true;

    createProgram(getVertexShaderString(), getFragmentShaderString());

}


void GlslFilter::printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    ALOGI("GL %s = %s\n", name, v);
}

void GlslFilter::checkGlError(const char *op) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        ALOGI("after %s() glError (0x%x)\n", op, error);
    }
}


GLuint GlslFilter::loadShader(GLenum shaderType, const char *pSource) {

    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    ALOGE("Could not compile shader %d:\n%s\n",
                          shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;

}

GLuint GlslFilter::createProgram(const char *pVertexSource, const char *pFragmentSource) {
    shaderProgram = 0;
    vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        ALOGE("create vertexShader error !");
        return 0;
    }

    pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        ALOGE("create pixelShader error !");
        return 0;
    }

    shaderProgram = glCreateProgram();
    if (shaderProgram) {
        glAttachShader(shaderProgram, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(shaderProgram, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(shaderProgram);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char *buf = (char *) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(shaderProgram, bufLength, NULL, buf);
                    ALOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }
    }

    // Bind attributes and uniforms
    texSamplerHandle = glGetUniformLocation(shaderProgram, "inputImageTexture");
    texCoordHandle = glGetAttribLocation(shaderProgram, "a_texcoord");
    posCoordHandle = glGetAttribLocation(shaderProgram, "a_position");
//    texCoordMatHandle = glGetUniformLocation(shaderProgram, "u_texture_mat");
//    modelViewMatHandle = glGetUniformLocation(shaderProgram, "u_model_view");


 //   texVertices = createVerticesBuffer(TEX_VERTICES_SURFACE_TEXTURE);
//    posVertices = createVerticesBuffer(POS_VERTICES);

    return shaderProgram;

}

void GlslFilter::renderBackground() {
    glClearColor(0.10588f, 0.109804f, 0.12157f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GlslFilter::process(Picture *in, Picture *out) {


    if (out == NULL) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }else {
        if (frameBufferObjectId == 0) {
            glGenFramebuffers(1, &frameBufferObjectId);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, out->texture);

        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, out->width,
                     out->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindFramebuffer(GL_FRAMEBUFFER,
                          frameBufferObjectId);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out->texture, 0);

        checkGlError("glBindFramebuffer");
    }

    glUseProgram(shaderProgram);
    checkGlError("glUseProgram");

    glViewport(0, 0, in->width, in->height);
    checkGlError("glViewport");
    glDisable(GL_BLEND);


    glVertexAttribPointer(texCoordHandle, 2, GL_FLOAT, GL_FALSE,
                          8, texVertices);
    glEnableVertexAttribArray(texCoordHandle);
    glVertexAttribPointer(posCoordHandle, 2, GL_FLOAT, GL_FALSE,
            8, posVertices);
    glEnableVertexAttribArray(posCoordHandle);
    checkGlError("vertex attribute setup");

    if (in != NULL && texSamplerHandle >= 0) {
        // Set the input texture
        glActiveTexture(GL_TEXTURE0);
        checkGlError("glActiveTexture");
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, in->texture);
        checkGlError("glBindTexture");
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER,
                        GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_EDGE);
        glUniform1i(texSamplerHandle, 0);
        checkGlError("texSamplerHandle");
    }


//    glUniformMatrix4fv(texCoordMatHandle, 1, GL_FALSE, mTextureMat);
//    checkGlError("texCoordMatHandle");
//    glUniformMatrix4fv(modelViewMatHandle, 1, GL_FALSE, mModelViewMat);
//    checkGlError("modelViewMatHandle");

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");
    glFinish();


    if (out != NULL) {
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    checkGlError("after process");


}

GLuint GlslFilter::createTexture() {
    GLuint texture;
    glGenTextures(1, &texture);
    checkGlError("glGenTextures");
    return texture;
}



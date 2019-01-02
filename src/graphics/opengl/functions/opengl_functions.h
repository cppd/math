/*
Copyright (C) 2017-2019 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Generated from glcorearb.h

// clang-format off

#pragma once

#include <GL/glcorearb.h>

#if defined(_WIN32)
#undef near
#undef far
#endif

namespace opengl_functions
{
        void init();

        extern PFNGLACTIVEPROGRAMEXTPROC                               glActiveProgramEXT;
        extern PFNGLACTIVESHADERPROGRAMPROC                            glActiveShaderProgram;
        extern PFNGLACTIVETEXTUREPROC                                  glActiveTexture;
        extern PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTELPROC            glApplyFramebufferAttachmentCMAAINTEL;
        extern PFNGLATTACHSHADERPROC                                   glAttachShader;
        extern PFNGLBEGINCONDITIONALRENDERPROC                         glBeginConditionalRender;
        extern PFNGLBEGINCONDITIONALRENDERNVPROC                       glBeginConditionalRenderNV;
        extern PFNGLBEGINPERFMONITORAMDPROC                            glBeginPerfMonitorAMD;
        extern PFNGLBEGINPERFQUERYINTELPROC                            glBeginPerfQueryINTEL;
        extern PFNGLBEGINQUERYPROC                                     glBeginQuery;
        extern PFNGLBEGINQUERYINDEXEDPROC                              glBeginQueryIndexed;
        extern PFNGLBEGINTRANSFORMFEEDBACKPROC                         glBeginTransformFeedback;
        extern PFNGLBINDATTRIBLOCATIONPROC                             glBindAttribLocation;
        extern PFNGLBINDBUFFERPROC                                     glBindBuffer;
        extern PFNGLBINDBUFFERBASEPROC                                 glBindBufferBase;
        extern PFNGLBINDBUFFERRANGEPROC                                glBindBufferRange;
        extern PFNGLBINDBUFFERSBASEPROC                                glBindBuffersBase;
        extern PFNGLBINDBUFFERSRANGEPROC                               glBindBuffersRange;
        extern PFNGLBINDFRAGDATALOCATIONPROC                           glBindFragDataLocation;
        extern PFNGLBINDFRAGDATALOCATIONINDEXEDPROC                    glBindFragDataLocationIndexed;
        extern PFNGLBINDFRAMEBUFFERPROC                                glBindFramebuffer;
        extern PFNGLBINDIMAGETEXTUREPROC                               glBindImageTexture;
        extern PFNGLBINDIMAGETEXTURESPROC                              glBindImageTextures;
        extern PFNGLBINDMULTITEXTUREEXTPROC                            glBindMultiTextureEXT;
        extern PFNGLBINDPROGRAMPIPELINEPROC                            glBindProgramPipeline;
        extern PFNGLBINDRENDERBUFFERPROC                               glBindRenderbuffer;
        extern PFNGLBINDSAMPLERPROC                                    glBindSampler;
        extern PFNGLBINDSAMPLERSPROC                                   glBindSamplers;
        extern PFNGLBINDSHADINGRATEIMAGENVPROC                         glBindShadingRateImageNV;
        extern PFNGLBINDTEXTUREPROC                                    glBindTexture;
        extern PFNGLBINDTEXTUREUNITPROC                                glBindTextureUnit;
        extern PFNGLBINDTEXTURESPROC                                   glBindTextures;
        extern PFNGLBINDTRANSFORMFEEDBACKPROC                          glBindTransformFeedback;
        extern PFNGLBINDVERTEXARRAYPROC                                glBindVertexArray;
        extern PFNGLBINDVERTEXBUFFERPROC                               glBindVertexBuffer;
        extern PFNGLBINDVERTEXBUFFERSPROC                              glBindVertexBuffers;
        extern PFNGLBLENDBARRIERKHRPROC                                glBlendBarrierKHR;
        extern PFNGLBLENDBARRIERNVPROC                                 glBlendBarrierNV;
        extern PFNGLBLENDCOLORPROC                                     glBlendColor;
        extern PFNGLBLENDEQUATIONPROC                                  glBlendEquation;
        extern PFNGLBLENDEQUATIONSEPARATEPROC                          glBlendEquationSeparate;
        extern PFNGLBLENDEQUATIONSEPARATEIPROC                         glBlendEquationSeparatei;
        extern PFNGLBLENDEQUATIONSEPARATEIARBPROC                      glBlendEquationSeparateiARB;
        extern PFNGLBLENDEQUATIONIPROC                                 glBlendEquationi;
        extern PFNGLBLENDEQUATIONIARBPROC                              glBlendEquationiARB;
        extern PFNGLBLENDFUNCPROC                                      glBlendFunc;
        extern PFNGLBLENDFUNCSEPARATEPROC                              glBlendFuncSeparate;
        extern PFNGLBLENDFUNCSEPARATEIPROC                             glBlendFuncSeparatei;
        extern PFNGLBLENDFUNCSEPARATEIARBPROC                          glBlendFuncSeparateiARB;
        extern PFNGLBLENDFUNCIPROC                                     glBlendFunci;
        extern PFNGLBLENDFUNCIARBPROC                                  glBlendFunciARB;
        extern PFNGLBLENDPARAMETERINVPROC                              glBlendParameteriNV;
        extern PFNGLBLITFRAMEBUFFERPROC                                glBlitFramebuffer;
        extern PFNGLBLITNAMEDFRAMEBUFFERPROC                           glBlitNamedFramebuffer;
        extern PFNGLBUFFERADDRESSRANGENVPROC                           glBufferAddressRangeNV;
        extern PFNGLBUFFERATTACHMEMORYNVPROC                           glBufferAttachMemoryNV;
        extern PFNGLBUFFERDATAPROC                                     glBufferData;
        extern PFNGLBUFFERPAGECOMMITMENTARBPROC                        glBufferPageCommitmentARB;
        extern PFNGLBUFFERSTORAGEPROC                                  glBufferStorage;
        extern PFNGLBUFFERSUBDATAPROC                                  glBufferSubData;
        extern PFNGLCALLCOMMANDLISTNVPROC                              glCallCommandListNV;
        extern PFNGLCHECKFRAMEBUFFERSTATUSPROC                         glCheckFramebufferStatus;
        extern PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC                    glCheckNamedFramebufferStatus;
        extern PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC                 glCheckNamedFramebufferStatusEXT;
        extern PFNGLCLAMPCOLORPROC                                     glClampColor;
        extern PFNGLCLEARPROC                                          glClear;
        extern PFNGLCLEARBUFFERDATAPROC                                glClearBufferData;
        extern PFNGLCLEARBUFFERSUBDATAPROC                             glClearBufferSubData;
        extern PFNGLCLEARBUFFERFIPROC                                  glClearBufferfi;
        extern PFNGLCLEARBUFFERFVPROC                                  glClearBufferfv;
        extern PFNGLCLEARBUFFERIVPROC                                  glClearBufferiv;
        extern PFNGLCLEARBUFFERUIVPROC                                 glClearBufferuiv;
        extern PFNGLCLEARCOLORPROC                                     glClearColor;
        extern PFNGLCLEARDEPTHPROC                                     glClearDepth;
        extern PFNGLCLEARDEPTHFPROC                                    glClearDepthf;
        extern PFNGLCLEARNAMEDBUFFERDATAPROC                           glClearNamedBufferData;
        extern PFNGLCLEARNAMEDBUFFERDATAEXTPROC                        glClearNamedBufferDataEXT;
        extern PFNGLCLEARNAMEDBUFFERSUBDATAPROC                        glClearNamedBufferSubData;
        extern PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC                     glClearNamedBufferSubDataEXT;
        extern PFNGLCLEARNAMEDFRAMEBUFFERFIPROC                        glClearNamedFramebufferfi;
        extern PFNGLCLEARNAMEDFRAMEBUFFERFVPROC                        glClearNamedFramebufferfv;
        extern PFNGLCLEARNAMEDFRAMEBUFFERIVPROC                        glClearNamedFramebufferiv;
        extern PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC                       glClearNamedFramebufferuiv;
        extern PFNGLCLEARSTENCILPROC                                   glClearStencil;
        extern PFNGLCLEARTEXIMAGEPROC                                  glClearTexImage;
        extern PFNGLCLEARTEXSUBIMAGEPROC                               glClearTexSubImage;
        extern PFNGLCLIENTATTRIBDEFAULTEXTPROC                         glClientAttribDefaultEXT;
        extern PFNGLCLIENTWAITSYNCPROC                                 glClientWaitSync;
        extern PFNGLCLIPCONTROLPROC                                    glClipControl;
        extern PFNGLCOLORFORMATNVPROC                                  glColorFormatNV;
        extern PFNGLCOLORMASKPROC                                      glColorMask;
        extern PFNGLCOLORMASKIPROC                                     glColorMaski;
        extern PFNGLCOMMANDLISTSEGMENTSNVPROC                          glCommandListSegmentsNV;
        extern PFNGLCOMPILECOMMANDLISTNVPROC                           glCompileCommandListNV;
        extern PFNGLCOMPILESHADERPROC                                  glCompileShader;
        extern PFNGLCOMPILESHADERINCLUDEARBPROC                        glCompileShaderIncludeARB;
        extern PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC                   glCompressedMultiTexImage1DEXT;
        extern PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC                   glCompressedMultiTexImage2DEXT;
        extern PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC                   glCompressedMultiTexImage3DEXT;
        extern PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC                glCompressedMultiTexSubImage1DEXT;
        extern PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC                glCompressedMultiTexSubImage2DEXT;
        extern PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC                glCompressedMultiTexSubImage3DEXT;
        extern PFNGLCOMPRESSEDTEXIMAGE1DPROC                           glCompressedTexImage1D;
        extern PFNGLCOMPRESSEDTEXIMAGE2DPROC                           glCompressedTexImage2D;
        extern PFNGLCOMPRESSEDTEXIMAGE3DPROC                           glCompressedTexImage3D;
        extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC                        glCompressedTexSubImage1D;
        extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC                        glCompressedTexSubImage2D;
        extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC                        glCompressedTexSubImage3D;
        extern PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC                    glCompressedTextureImage1DEXT;
        extern PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC                    glCompressedTextureImage2DEXT;
        extern PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC                    glCompressedTextureImage3DEXT;
        extern PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC                    glCompressedTextureSubImage1D;
        extern PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC                 glCompressedTextureSubImage1DEXT;
        extern PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC                    glCompressedTextureSubImage2D;
        extern PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC                 glCompressedTextureSubImage2DEXT;
        extern PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC                    glCompressedTextureSubImage3D;
        extern PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC                 glCompressedTextureSubImage3DEXT;
        extern PFNGLCONSERVATIVERASTERPARAMETERFNVPROC                 glConservativeRasterParameterfNV;
        extern PFNGLCONSERVATIVERASTERPARAMETERINVPROC                 glConservativeRasterParameteriNV;
        extern PFNGLCOPYBUFFERSUBDATAPROC                              glCopyBufferSubData;
        extern PFNGLCOPYIMAGESUBDATAPROC                               glCopyImageSubData;
        extern PFNGLCOPYMULTITEXIMAGE1DEXTPROC                         glCopyMultiTexImage1DEXT;
        extern PFNGLCOPYMULTITEXIMAGE2DEXTPROC                         glCopyMultiTexImage2DEXT;
        extern PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC                      glCopyMultiTexSubImage1DEXT;
        extern PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC                      glCopyMultiTexSubImage2DEXT;
        extern PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC                      glCopyMultiTexSubImage3DEXT;
        extern PFNGLCOPYNAMEDBUFFERSUBDATAPROC                         glCopyNamedBufferSubData;
        extern PFNGLCOPYPATHNVPROC                                     glCopyPathNV;
        extern PFNGLCOPYTEXIMAGE1DPROC                                 glCopyTexImage1D;
        extern PFNGLCOPYTEXIMAGE2DPROC                                 glCopyTexImage2D;
        extern PFNGLCOPYTEXSUBIMAGE1DPROC                              glCopyTexSubImage1D;
        extern PFNGLCOPYTEXSUBIMAGE2DPROC                              glCopyTexSubImage2D;
        extern PFNGLCOPYTEXSUBIMAGE3DPROC                              glCopyTexSubImage3D;
        extern PFNGLCOPYTEXTUREIMAGE1DEXTPROC                          glCopyTextureImage1DEXT;
        extern PFNGLCOPYTEXTUREIMAGE2DEXTPROC                          glCopyTextureImage2DEXT;
        extern PFNGLCOPYTEXTURESUBIMAGE1DPROC                          glCopyTextureSubImage1D;
        extern PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC                       glCopyTextureSubImage1DEXT;
        extern PFNGLCOPYTEXTURESUBIMAGE2DPROC                          glCopyTextureSubImage2D;
        extern PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC                       glCopyTextureSubImage2DEXT;
        extern PFNGLCOPYTEXTURESUBIMAGE3DPROC                          glCopyTextureSubImage3D;
        extern PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC                       glCopyTextureSubImage3DEXT;
        extern PFNGLCOVERFILLPATHINSTANCEDNVPROC                       glCoverFillPathInstancedNV;
        extern PFNGLCOVERFILLPATHNVPROC                                glCoverFillPathNV;
        extern PFNGLCOVERSTROKEPATHINSTANCEDNVPROC                     glCoverStrokePathInstancedNV;
        extern PFNGLCOVERSTROKEPATHNVPROC                              glCoverStrokePathNV;
        extern PFNGLCOVERAGEMODULATIONNVPROC                           glCoverageModulationNV;
        extern PFNGLCOVERAGEMODULATIONTABLENVPROC                      glCoverageModulationTableNV;
        extern PFNGLCREATEBUFFERSPROC                                  glCreateBuffers;
        extern PFNGLCREATECOMMANDLISTSNVPROC                           glCreateCommandListsNV;
        extern PFNGLCREATEFRAMEBUFFERSPROC                             glCreateFramebuffers;
        extern PFNGLCREATEPERFQUERYINTELPROC                           glCreatePerfQueryINTEL;
        extern PFNGLCREATEPROGRAMPROC                                  glCreateProgram;
        extern PFNGLCREATEPROGRAMPIPELINESPROC                         glCreateProgramPipelines;
        extern PFNGLCREATEQUERIESPROC                                  glCreateQueries;
        extern PFNGLCREATERENDERBUFFERSPROC                            glCreateRenderbuffers;
        extern PFNGLCREATESAMPLERSPROC                                 glCreateSamplers;
        extern PFNGLCREATESHADERPROC                                   glCreateShader;
        extern PFNGLCREATESHADERPROGRAMEXTPROC                         glCreateShaderProgramEXT;
        extern PFNGLCREATESHADERPROGRAMVPROC                           glCreateShaderProgramv;
        extern PFNGLCREATESTATESNVPROC                                 glCreateStatesNV;
        extern PFNGLCREATESYNCFROMCLEVENTARBPROC                       glCreateSyncFromCLeventARB;
        extern PFNGLCREATETEXTURESPROC                                 glCreateTextures;
        extern PFNGLCREATETRANSFORMFEEDBACKSPROC                       glCreateTransformFeedbacks;
        extern PFNGLCREATEVERTEXARRAYSPROC                             glCreateVertexArrays;
        extern PFNGLCULLFACEPROC                                       glCullFace;
        extern PFNGLDEBUGMESSAGECALLBACKPROC                           glDebugMessageCallback;
        extern PFNGLDEBUGMESSAGECALLBACKARBPROC                        glDebugMessageCallbackARB;
        extern PFNGLDEBUGMESSAGECONTROLPROC                            glDebugMessageControl;
        extern PFNGLDEBUGMESSAGECONTROLARBPROC                         glDebugMessageControlARB;
        extern PFNGLDEBUGMESSAGEINSERTPROC                             glDebugMessageInsert;
        extern PFNGLDEBUGMESSAGEINSERTARBPROC                          glDebugMessageInsertARB;
        extern PFNGLDELETEBUFFERSPROC                                  glDeleteBuffers;
        extern PFNGLDELETECOMMANDLISTSNVPROC                           glDeleteCommandListsNV;
        extern PFNGLDELETEFRAMEBUFFERSPROC                             glDeleteFramebuffers;
        extern PFNGLDELETENAMEDSTRINGARBPROC                           glDeleteNamedStringARB;
        extern PFNGLDELETEPATHSNVPROC                                  glDeletePathsNV;
        extern PFNGLDELETEPERFMONITORSAMDPROC                          glDeletePerfMonitorsAMD;
        extern PFNGLDELETEPERFQUERYINTELPROC                           glDeletePerfQueryINTEL;
        extern PFNGLDELETEPROGRAMPROC                                  glDeleteProgram;
        extern PFNGLDELETEPROGRAMPIPELINESPROC                         glDeleteProgramPipelines;
        extern PFNGLDELETEQUERIESPROC                                  glDeleteQueries;
        extern PFNGLDELETERENDERBUFFERSPROC                            glDeleteRenderbuffers;
        extern PFNGLDELETESAMPLERSPROC                                 glDeleteSamplers;
        extern PFNGLDELETESHADERPROC                                   glDeleteShader;
        extern PFNGLDELETESTATESNVPROC                                 glDeleteStatesNV;
        extern PFNGLDELETESYNCPROC                                     glDeleteSync;
        extern PFNGLDELETETEXTURESPROC                                 glDeleteTextures;
        extern PFNGLDELETETRANSFORMFEEDBACKSPROC                       glDeleteTransformFeedbacks;
        extern PFNGLDELETEVERTEXARRAYSPROC                             glDeleteVertexArrays;
        extern PFNGLDEPTHFUNCPROC                                      glDepthFunc;
        extern PFNGLDEPTHMASKPROC                                      glDepthMask;
        extern PFNGLDEPTHRANGEPROC                                     glDepthRange;
        extern PFNGLDEPTHRANGEARRAYVPROC                               glDepthRangeArrayv;
        extern PFNGLDEPTHRANGEINDEXEDPROC                              glDepthRangeIndexed;
        extern PFNGLDEPTHRANGEFPROC                                    glDepthRangef;
        extern PFNGLDETACHSHADERPROC                                   glDetachShader;
        extern PFNGLDISABLEPROC                                        glDisable;
        extern PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC                   glDisableClientStateIndexedEXT;
        extern PFNGLDISABLECLIENTSTATEIEXTPROC                         glDisableClientStateiEXT;
        extern PFNGLDISABLEINDEXEDEXTPROC                              glDisableIndexedEXT;
        extern PFNGLDISABLEVERTEXARRAYATTRIBPROC                       glDisableVertexArrayAttrib;
        extern PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC                    glDisableVertexArrayAttribEXT;
        extern PFNGLDISABLEVERTEXARRAYEXTPROC                          glDisableVertexArrayEXT;
        extern PFNGLDISABLEVERTEXATTRIBARRAYPROC                       glDisableVertexAttribArray;
        extern PFNGLDISABLEIPROC                                       glDisablei;
        extern PFNGLDISPATCHCOMPUTEPROC                                glDispatchCompute;
        extern PFNGLDISPATCHCOMPUTEGROUPSIZEARBPROC                    glDispatchComputeGroupSizeARB;
        extern PFNGLDISPATCHCOMPUTEINDIRECTPROC                        glDispatchComputeIndirect;
        extern PFNGLDRAWARRAYSPROC                                     glDrawArrays;
        extern PFNGLDRAWARRAYSINDIRECTPROC                             glDrawArraysIndirect;
        extern PFNGLDRAWARRAYSINSTANCEDPROC                            glDrawArraysInstanced;
        extern PFNGLDRAWARRAYSINSTANCEDARBPROC                         glDrawArraysInstancedARB;
        extern PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC                glDrawArraysInstancedBaseInstance;
        extern PFNGLDRAWARRAYSINSTANCEDEXTPROC                         glDrawArraysInstancedEXT;
        extern PFNGLDRAWBUFFERPROC                                     glDrawBuffer;
        extern PFNGLDRAWBUFFERSPROC                                    glDrawBuffers;
        extern PFNGLDRAWCOMMANDSADDRESSNVPROC                          glDrawCommandsAddressNV;
        extern PFNGLDRAWCOMMANDSNVPROC                                 glDrawCommandsNV;
        extern PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC                    glDrawCommandsStatesAddressNV;
        extern PFNGLDRAWCOMMANDSSTATESNVPROC                           glDrawCommandsStatesNV;
        extern PFNGLDRAWELEMENTSPROC                                   glDrawElements;
        extern PFNGLDRAWELEMENTSBASEVERTEXPROC                         glDrawElementsBaseVertex;
        extern PFNGLDRAWELEMENTSINDIRECTPROC                           glDrawElementsIndirect;
        extern PFNGLDRAWELEMENTSINSTANCEDPROC                          glDrawElementsInstanced;
        extern PFNGLDRAWELEMENTSINSTANCEDARBPROC                       glDrawElementsInstancedARB;
        extern PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC              glDrawElementsInstancedBaseInstance;
        extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC                glDrawElementsInstancedBaseVertex;
        extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC    glDrawElementsInstancedBaseVertexBaseInstance;
        extern PFNGLDRAWELEMENTSINSTANCEDEXTPROC                       glDrawElementsInstancedEXT;
        extern PFNGLDRAWMESHTASKSINDIRECTNVPROC                        glDrawMeshTasksIndirectNV;
        extern PFNGLDRAWMESHTASKSNVPROC                                glDrawMeshTasksNV;
        extern PFNGLDRAWRANGEELEMENTSPROC                              glDrawRangeElements;
        extern PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC                    glDrawRangeElementsBaseVertex;
        extern PFNGLDRAWTRANSFORMFEEDBACKPROC                          glDrawTransformFeedback;
        extern PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC                 glDrawTransformFeedbackInstanced;
        extern PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC                    glDrawTransformFeedbackStream;
        extern PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC           glDrawTransformFeedbackStreamInstanced;
        extern PFNGLDRAWVKIMAGENVPROC                                  glDrawVkImageNV;
        extern PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC                    glEGLImageTargetTexStorageEXT;
        extern PFNGLEGLIMAGETARGETTEXTURESTORAGEEXTPROC                glEGLImageTargetTextureStorageEXT;
        extern PFNGLEDGEFLAGFORMATNVPROC                               glEdgeFlagFormatNV;
        extern PFNGLENABLEPROC                                         glEnable;
        extern PFNGLENABLECLIENTSTATEINDEXEDEXTPROC                    glEnableClientStateIndexedEXT;
        extern PFNGLENABLECLIENTSTATEIEXTPROC                          glEnableClientStateiEXT;
        extern PFNGLENABLEINDEXEDEXTPROC                               glEnableIndexedEXT;
        extern PFNGLENABLEVERTEXARRAYATTRIBPROC                        glEnableVertexArrayAttrib;
        extern PFNGLENABLEVERTEXARRAYATTRIBEXTPROC                     glEnableVertexArrayAttribEXT;
        extern PFNGLENABLEVERTEXARRAYEXTPROC                           glEnableVertexArrayEXT;
        extern PFNGLENABLEVERTEXATTRIBARRAYPROC                        glEnableVertexAttribArray;
        extern PFNGLENABLEIPROC                                        glEnablei;
        extern PFNGLENDCONDITIONALRENDERPROC                           glEndConditionalRender;
        extern PFNGLENDCONDITIONALRENDERNVPROC                         glEndConditionalRenderNV;
        extern PFNGLENDPERFMONITORAMDPROC                              glEndPerfMonitorAMD;
        extern PFNGLENDPERFQUERYINTELPROC                              glEndPerfQueryINTEL;
        extern PFNGLENDQUERYPROC                                       glEndQuery;
        extern PFNGLENDQUERYINDEXEDPROC                                glEndQueryIndexed;
        extern PFNGLENDTRANSFORMFEEDBACKPROC                           glEndTransformFeedback;
        extern PFNGLEVALUATEDEPTHVALUESARBPROC                         glEvaluateDepthValuesARB;
        extern PFNGLFENCESYNCPROC                                      glFenceSync;
        extern PFNGLFINISHPROC                                         glFinish;
        extern PFNGLFLUSHPROC                                          glFlush;
        extern PFNGLFLUSHMAPPEDBUFFERRANGEPROC                         glFlushMappedBufferRange;
        extern PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC                    glFlushMappedNamedBufferRange;
        extern PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC                 glFlushMappedNamedBufferRangeEXT;
        extern PFNGLFOGCOORDFORMATNVPROC                               glFogCoordFormatNV;
        extern PFNGLFRAGMENTCOVERAGECOLORNVPROC                        glFragmentCoverageColorNV;
        extern PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC                       glFramebufferDrawBufferEXT;
        extern PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC                      glFramebufferDrawBuffersEXT;
        extern PFNGLFRAMEBUFFERFETCHBARRIEREXTPROC                     glFramebufferFetchBarrierEXT;
        extern PFNGLFRAMEBUFFERPARAMETERIPROC                          glFramebufferParameteri;
        extern PFNGLFRAMEBUFFERREADBUFFEREXTPROC                       glFramebufferReadBufferEXT;
        extern PFNGLFRAMEBUFFERRENDERBUFFERPROC                        glFramebufferRenderbuffer;
        extern PFNGLFRAMEBUFFERSAMPLELOCATIONSFVARBPROC                glFramebufferSampleLocationsfvARB;
        extern PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNVPROC                 glFramebufferSampleLocationsfvNV;
        extern PFNGLFRAMEBUFFERTEXTUREPROC                             glFramebufferTexture;
        extern PFNGLFRAMEBUFFERTEXTURE1DPROC                           glFramebufferTexture1D;
        extern PFNGLFRAMEBUFFERTEXTURE2DPROC                           glFramebufferTexture2D;
        extern PFNGLFRAMEBUFFERTEXTURE3DPROC                           glFramebufferTexture3D;
        extern PFNGLFRAMEBUFFERTEXTUREARBPROC                          glFramebufferTextureARB;
        extern PFNGLFRAMEBUFFERTEXTUREFACEARBPROC                      glFramebufferTextureFaceARB;
        extern PFNGLFRAMEBUFFERTEXTURELAYERPROC                        glFramebufferTextureLayer;
        extern PFNGLFRAMEBUFFERTEXTURELAYERARBPROC                     glFramebufferTextureLayerARB;
        extern PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC                 glFramebufferTextureMultiviewOVR;
        extern PFNGLFRONTFACEPROC                                      glFrontFace;
        extern PFNGLGENBUFFERSPROC                                     glGenBuffers;
        extern PFNGLGENFRAMEBUFFERSPROC                                glGenFramebuffers;
        extern PFNGLGENPATHSNVPROC                                     glGenPathsNV;
        extern PFNGLGENPERFMONITORSAMDPROC                             glGenPerfMonitorsAMD;
        extern PFNGLGENPROGRAMPIPELINESPROC                            glGenProgramPipelines;
        extern PFNGLGENQUERIESPROC                                     glGenQueries;
        extern PFNGLGENRENDERBUFFERSPROC                               glGenRenderbuffers;
        extern PFNGLGENSAMPLERSPROC                                    glGenSamplers;
        extern PFNGLGENTEXTURESPROC                                    glGenTextures;
        extern PFNGLGENTRANSFORMFEEDBACKSPROC                          glGenTransformFeedbacks;
        extern PFNGLGENVERTEXARRAYSPROC                                glGenVertexArrays;
        extern PFNGLGENERATEMIPMAPPROC                                 glGenerateMipmap;
        extern PFNGLGENERATEMULTITEXMIPMAPEXTPROC                      glGenerateMultiTexMipmapEXT;
        extern PFNGLGENERATETEXTUREMIPMAPPROC                          glGenerateTextureMipmap;
        extern PFNGLGENERATETEXTUREMIPMAPEXTPROC                       glGenerateTextureMipmapEXT;
        extern PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC                 glGetActiveAtomicCounterBufferiv;
        extern PFNGLGETACTIVEATTRIBPROC                                glGetActiveAttrib;
        extern PFNGLGETACTIVESUBROUTINENAMEPROC                        glGetActiveSubroutineName;
        extern PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC                 glGetActiveSubroutineUniformName;
        extern PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC                   glGetActiveSubroutineUniformiv;
        extern PFNGLGETACTIVEUNIFORMPROC                               glGetActiveUniform;
        extern PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC                      glGetActiveUniformBlockName;
        extern PFNGLGETACTIVEUNIFORMBLOCKIVPROC                        glGetActiveUniformBlockiv;
        extern PFNGLGETACTIVEUNIFORMNAMEPROC                           glGetActiveUniformName;
        extern PFNGLGETACTIVEUNIFORMSIVPROC                            glGetActiveUniformsiv;
        extern PFNGLGETATTACHEDSHADERSPROC                             glGetAttachedShaders;
        extern PFNGLGETATTRIBLOCATIONPROC                              glGetAttribLocation;
        extern PFNGLGETBOOLEANINDEXEDVEXTPROC                          glGetBooleanIndexedvEXT;
        extern PFNGLGETBOOLEANI_VPROC                                  glGetBooleani_v;
        extern PFNGLGETBOOLEANVPROC                                    glGetBooleanv;
        extern PFNGLGETBUFFERPARAMETERI64VPROC                         glGetBufferParameteri64v;
        extern PFNGLGETBUFFERPARAMETERIVPROC                           glGetBufferParameteriv;
        extern PFNGLGETBUFFERPARAMETERUI64VNVPROC                      glGetBufferParameterui64vNV;
        extern PFNGLGETBUFFERPOINTERVPROC                              glGetBufferPointerv;
        extern PFNGLGETBUFFERSUBDATAPROC                               glGetBufferSubData;
        extern PFNGLGETCOMMANDHEADERNVPROC                             glGetCommandHeaderNV;
        extern PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC                  glGetCompressedMultiTexImageEXT;
        extern PFNGLGETCOMPRESSEDTEXIMAGEPROC                          glGetCompressedTexImage;
        extern PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC                      glGetCompressedTextureImage;
        extern PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC                   glGetCompressedTextureImageEXT;
        extern PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC                   glGetCompressedTextureSubImage;
        extern PFNGLGETCOVERAGEMODULATIONTABLENVPROC                   glGetCoverageModulationTableNV;
        extern PFNGLGETDEBUGMESSAGELOGPROC                             glGetDebugMessageLog;
        extern PFNGLGETDEBUGMESSAGELOGARBPROC                          glGetDebugMessageLogARB;
        extern PFNGLGETDOUBLEINDEXEDVEXTPROC                           glGetDoubleIndexedvEXT;
        extern PFNGLGETDOUBLEI_VPROC                                   glGetDoublei_v;
        extern PFNGLGETDOUBLEI_VEXTPROC                                glGetDoublei_vEXT;
        extern PFNGLGETDOUBLEVPROC                                     glGetDoublev;
        extern PFNGLGETERRORPROC                                       glGetError;
        extern PFNGLGETFIRSTPERFQUERYIDINTELPROC                       glGetFirstPerfQueryIdINTEL;
        extern PFNGLGETFLOATINDEXEDVEXTPROC                            glGetFloatIndexedvEXT;
        extern PFNGLGETFLOATI_VPROC                                    glGetFloati_v;
        extern PFNGLGETFLOATI_VEXTPROC                                 glGetFloati_vEXT;
        extern PFNGLGETFLOATVPROC                                      glGetFloatv;
        extern PFNGLGETFRAGDATAINDEXPROC                               glGetFragDataIndex;
        extern PFNGLGETFRAGDATALOCATIONPROC                            glGetFragDataLocation;
        extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC            glGetFramebufferAttachmentParameteriv;
        extern PFNGLGETFRAMEBUFFERPARAMETERIVPROC                      glGetFramebufferParameteriv;
        extern PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC                   glGetFramebufferParameterivEXT;
        extern PFNGLGETGRAPHICSRESETSTATUSPROC                         glGetGraphicsResetStatus;
        extern PFNGLGETGRAPHICSRESETSTATUSARBPROC                      glGetGraphicsResetStatusARB;
        extern PFNGLGETIMAGEHANDLEARBPROC                              glGetImageHandleARB;
        extern PFNGLGETIMAGEHANDLENVPROC                               glGetImageHandleNV;
        extern PFNGLGETINTEGER64I_VPROC                                glGetInteger64i_v;
        extern PFNGLGETINTEGER64VPROC                                  glGetInteger64v;
        extern PFNGLGETINTEGERINDEXEDVEXTPROC                          glGetIntegerIndexedvEXT;
        extern PFNGLGETINTEGERI_VPROC                                  glGetIntegeri_v;
        extern PFNGLGETINTEGERUI64I_VNVPROC                            glGetIntegerui64i_vNV;
        extern PFNGLGETINTEGERUI64VNVPROC                              glGetIntegerui64vNV;
        extern PFNGLGETINTEGERVPROC                                    glGetIntegerv;
        extern PFNGLGETINTERNALFORMATSAMPLEIVNVPROC                    glGetInternalformatSampleivNV;
        extern PFNGLGETINTERNALFORMATI64VPROC                          glGetInternalformati64v;
        extern PFNGLGETINTERNALFORMATIVPROC                            glGetInternalformativ;
        extern PFNGLGETMEMORYOBJECTDETACHEDRESOURCESUIVNVPROC          glGetMemoryObjectDetachedResourcesuivNV;
        extern PFNGLGETMULTITEXENVFVEXTPROC                            glGetMultiTexEnvfvEXT;
        extern PFNGLGETMULTITEXENVIVEXTPROC                            glGetMultiTexEnvivEXT;
        extern PFNGLGETMULTITEXGENDVEXTPROC                            glGetMultiTexGendvEXT;
        extern PFNGLGETMULTITEXGENFVEXTPROC                            glGetMultiTexGenfvEXT;
        extern PFNGLGETMULTITEXGENIVEXTPROC                            glGetMultiTexGenivEXT;
        extern PFNGLGETMULTITEXIMAGEEXTPROC                            glGetMultiTexImageEXT;
        extern PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC                 glGetMultiTexLevelParameterfvEXT;
        extern PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC                 glGetMultiTexLevelParameterivEXT;
        extern PFNGLGETMULTITEXPARAMETERIIVEXTPROC                     glGetMultiTexParameterIivEXT;
        extern PFNGLGETMULTITEXPARAMETERIUIVEXTPROC                    glGetMultiTexParameterIuivEXT;
        extern PFNGLGETMULTITEXPARAMETERFVEXTPROC                      glGetMultiTexParameterfvEXT;
        extern PFNGLGETMULTITEXPARAMETERIVEXTPROC                      glGetMultiTexParameterivEXT;
        extern PFNGLGETMULTISAMPLEFVPROC                               glGetMultisamplefv;
        extern PFNGLGETNAMEDBUFFERPARAMETERI64VPROC                    glGetNamedBufferParameteri64v;
        extern PFNGLGETNAMEDBUFFERPARAMETERIVPROC                      glGetNamedBufferParameteriv;
        extern PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC                   glGetNamedBufferParameterivEXT;
        extern PFNGLGETNAMEDBUFFERPARAMETERUI64VNVPROC                 glGetNamedBufferParameterui64vNV;
        extern PFNGLGETNAMEDBUFFERPOINTERVPROC                         glGetNamedBufferPointerv;
        extern PFNGLGETNAMEDBUFFERPOINTERVEXTPROC                      glGetNamedBufferPointervEXT;
        extern PFNGLGETNAMEDBUFFERSUBDATAPROC                          glGetNamedBufferSubData;
        extern PFNGLGETNAMEDBUFFERSUBDATAEXTPROC                       glGetNamedBufferSubDataEXT;
        extern PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC       glGetNamedFramebufferAttachmentParameteriv;
        extern PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC    glGetNamedFramebufferAttachmentParameterivEXT;
        extern PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC                 glGetNamedFramebufferParameteriv;
        extern PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC              glGetNamedFramebufferParameterivEXT;
        extern PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC            glGetNamedProgramLocalParameterIivEXT;
        extern PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC           glGetNamedProgramLocalParameterIuivEXT;
        extern PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC             glGetNamedProgramLocalParameterdvEXT;
        extern PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC             glGetNamedProgramLocalParameterfvEXT;
        extern PFNGLGETNAMEDPROGRAMSTRINGEXTPROC                       glGetNamedProgramStringEXT;
        extern PFNGLGETNAMEDPROGRAMIVEXTPROC                           glGetNamedProgramivEXT;
        extern PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC                glGetNamedRenderbufferParameteriv;
        extern PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC             glGetNamedRenderbufferParameterivEXT;
        extern PFNGLGETNAMEDSTRINGARBPROC                              glGetNamedStringARB;
        extern PFNGLGETNAMEDSTRINGIVARBPROC                            glGetNamedStringivARB;
        extern PFNGLGETNEXTPERFQUERYIDINTELPROC                        glGetNextPerfQueryIdINTEL;
        extern PFNGLGETOBJECTLABELPROC                                 glGetObjectLabel;
        extern PFNGLGETOBJECTLABELEXTPROC                              glGetObjectLabelEXT;
        extern PFNGLGETOBJECTPTRLABELPROC                              glGetObjectPtrLabel;
        extern PFNGLGETPATHCOMMANDSNVPROC                              glGetPathCommandsNV;
        extern PFNGLGETPATHCOORDSNVPROC                                glGetPathCoordsNV;
        extern PFNGLGETPATHDASHARRAYNVPROC                             glGetPathDashArrayNV;
        extern PFNGLGETPATHLENGTHNVPROC                                glGetPathLengthNV;
        extern PFNGLGETPATHMETRICRANGENVPROC                           glGetPathMetricRangeNV;
        extern PFNGLGETPATHMETRICSNVPROC                               glGetPathMetricsNV;
        extern PFNGLGETPATHPARAMETERFVNVPROC                           glGetPathParameterfvNV;
        extern PFNGLGETPATHPARAMETERIVNVPROC                           glGetPathParameterivNV;
        extern PFNGLGETPATHSPACINGNVPROC                               glGetPathSpacingNV;
        extern PFNGLGETPERFCOUNTERINFOINTELPROC                        glGetPerfCounterInfoINTEL;
        extern PFNGLGETPERFMONITORCOUNTERDATAAMDPROC                   glGetPerfMonitorCounterDataAMD;
        extern PFNGLGETPERFMONITORCOUNTERINFOAMDPROC                   glGetPerfMonitorCounterInfoAMD;
        extern PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC                 glGetPerfMonitorCounterStringAMD;
        extern PFNGLGETPERFMONITORCOUNTERSAMDPROC                      glGetPerfMonitorCountersAMD;
        extern PFNGLGETPERFMONITORGROUPSTRINGAMDPROC                   glGetPerfMonitorGroupStringAMD;
        extern PFNGLGETPERFMONITORGROUPSAMDPROC                        glGetPerfMonitorGroupsAMD;
        extern PFNGLGETPERFQUERYDATAINTELPROC                          glGetPerfQueryDataINTEL;
        extern PFNGLGETPERFQUERYIDBYNAMEINTELPROC                      glGetPerfQueryIdByNameINTEL;
        extern PFNGLGETPERFQUERYINFOINTELPROC                          glGetPerfQueryInfoINTEL;
        extern PFNGLGETPOINTERINDEXEDVEXTPROC                          glGetPointerIndexedvEXT;
        extern PFNGLGETPOINTERI_VEXTPROC                               glGetPointeri_vEXT;
        extern PFNGLGETPOINTERVPROC                                    glGetPointerv;
        extern PFNGLGETPROGRAMBINARYPROC                               glGetProgramBinary;
        extern PFNGLGETPROGRAMINFOLOGPROC                              glGetProgramInfoLog;
        extern PFNGLGETPROGRAMINTERFACEIVPROC                          glGetProgramInterfaceiv;
        extern PFNGLGETPROGRAMPIPELINEINFOLOGPROC                      glGetProgramPipelineInfoLog;
        extern PFNGLGETPROGRAMPIPELINEIVPROC                           glGetProgramPipelineiv;
        extern PFNGLGETPROGRAMRESOURCEINDEXPROC                        glGetProgramResourceIndex;
        extern PFNGLGETPROGRAMRESOURCELOCATIONPROC                     glGetProgramResourceLocation;
        extern PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC                glGetProgramResourceLocationIndex;
        extern PFNGLGETPROGRAMRESOURCENAMEPROC                         glGetProgramResourceName;
        extern PFNGLGETPROGRAMRESOURCEFVNVPROC                         glGetProgramResourcefvNV;
        extern PFNGLGETPROGRAMRESOURCEIVPROC                           glGetProgramResourceiv;
        extern PFNGLGETPROGRAMSTAGEIVPROC                              glGetProgramStageiv;
        extern PFNGLGETPROGRAMIVPROC                                   glGetProgramiv;
        extern PFNGLGETQUERYBUFFEROBJECTI64VPROC                       glGetQueryBufferObjecti64v;
        extern PFNGLGETQUERYBUFFEROBJECTIVPROC                         glGetQueryBufferObjectiv;
        extern PFNGLGETQUERYBUFFEROBJECTUI64VPROC                      glGetQueryBufferObjectui64v;
        extern PFNGLGETQUERYBUFFEROBJECTUIVPROC                        glGetQueryBufferObjectuiv;
        extern PFNGLGETQUERYINDEXEDIVPROC                              glGetQueryIndexediv;
        extern PFNGLGETQUERYOBJECTI64VPROC                             glGetQueryObjecti64v;
        extern PFNGLGETQUERYOBJECTIVPROC                               glGetQueryObjectiv;
        extern PFNGLGETQUERYOBJECTUI64VPROC                            glGetQueryObjectui64v;
        extern PFNGLGETQUERYOBJECTUIVPROC                              glGetQueryObjectuiv;
        extern PFNGLGETQUERYIVPROC                                     glGetQueryiv;
        extern PFNGLGETRENDERBUFFERPARAMETERIVPROC                     glGetRenderbufferParameteriv;
        extern PFNGLGETSAMPLERPARAMETERIIVPROC                         glGetSamplerParameterIiv;
        extern PFNGLGETSAMPLERPARAMETERIUIVPROC                        glGetSamplerParameterIuiv;
        extern PFNGLGETSAMPLERPARAMETERFVPROC                          glGetSamplerParameterfv;
        extern PFNGLGETSAMPLERPARAMETERIVPROC                          glGetSamplerParameteriv;
        extern PFNGLGETSHADERINFOLOGPROC                               glGetShaderInfoLog;
        extern PFNGLGETSHADERPRECISIONFORMATPROC                       glGetShaderPrecisionFormat;
        extern PFNGLGETSHADERSOURCEPROC                                glGetShaderSource;
        extern PFNGLGETSHADERIVPROC                                    glGetShaderiv;
        extern PFNGLGETSHADINGRATEIMAGEPALETTENVPROC                   glGetShadingRateImagePaletteNV;
        extern PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC               glGetShadingRateSampleLocationivNV;
        extern PFNGLGETSTAGEINDEXNVPROC                                glGetStageIndexNV;
        extern PFNGLGETSTRINGPROC                                      glGetString;
        extern PFNGLGETSTRINGIPROC                                     glGetStringi;
        extern PFNGLGETSUBROUTINEINDEXPROC                             glGetSubroutineIndex;
        extern PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC                   glGetSubroutineUniformLocation;
        extern PFNGLGETSYNCIVPROC                                      glGetSynciv;
        extern PFNGLGETTEXIMAGEPROC                                    glGetTexImage;
        extern PFNGLGETTEXLEVELPARAMETERFVPROC                         glGetTexLevelParameterfv;
        extern PFNGLGETTEXLEVELPARAMETERIVPROC                         glGetTexLevelParameteriv;
        extern PFNGLGETTEXPARAMETERIIVPROC                             glGetTexParameterIiv;
        extern PFNGLGETTEXPARAMETERIUIVPROC                            glGetTexParameterIuiv;
        extern PFNGLGETTEXPARAMETERFVPROC                              glGetTexParameterfv;
        extern PFNGLGETTEXPARAMETERIVPROC                              glGetTexParameteriv;
        extern PFNGLGETTEXTUREHANDLEARBPROC                            glGetTextureHandleARB;
        extern PFNGLGETTEXTUREHANDLENVPROC                             glGetTextureHandleNV;
        extern PFNGLGETTEXTUREIMAGEPROC                                glGetTextureImage;
        extern PFNGLGETTEXTUREIMAGEEXTPROC                             glGetTextureImageEXT;
        extern PFNGLGETTEXTURELEVELPARAMETERFVPROC                     glGetTextureLevelParameterfv;
        extern PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC                  glGetTextureLevelParameterfvEXT;
        extern PFNGLGETTEXTURELEVELPARAMETERIVPROC                     glGetTextureLevelParameteriv;
        extern PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC                  glGetTextureLevelParameterivEXT;
        extern PFNGLGETTEXTUREPARAMETERIIVPROC                         glGetTextureParameterIiv;
        extern PFNGLGETTEXTUREPARAMETERIIVEXTPROC                      glGetTextureParameterIivEXT;
        extern PFNGLGETTEXTUREPARAMETERIUIVPROC                        glGetTextureParameterIuiv;
        extern PFNGLGETTEXTUREPARAMETERIUIVEXTPROC                     glGetTextureParameterIuivEXT;
        extern PFNGLGETTEXTUREPARAMETERFVPROC                          glGetTextureParameterfv;
        extern PFNGLGETTEXTUREPARAMETERFVEXTPROC                       glGetTextureParameterfvEXT;
        extern PFNGLGETTEXTUREPARAMETERIVPROC                          glGetTextureParameteriv;
        extern PFNGLGETTEXTUREPARAMETERIVEXTPROC                       glGetTextureParameterivEXT;
        extern PFNGLGETTEXTURESAMPLERHANDLEARBPROC                     glGetTextureSamplerHandleARB;
        extern PFNGLGETTEXTURESAMPLERHANDLENVPROC                      glGetTextureSamplerHandleNV;
        extern PFNGLGETTEXTURESUBIMAGEPROC                             glGetTextureSubImage;
        extern PFNGLGETTRANSFORMFEEDBACKVARYINGPROC                    glGetTransformFeedbackVarying;
        extern PFNGLGETTRANSFORMFEEDBACKI64_VPROC                      glGetTransformFeedbacki64_v;
        extern PFNGLGETTRANSFORMFEEDBACKI_VPROC                        glGetTransformFeedbacki_v;
        extern PFNGLGETTRANSFORMFEEDBACKIVPROC                         glGetTransformFeedbackiv;
        extern PFNGLGETUNIFORMBLOCKINDEXPROC                           glGetUniformBlockIndex;
        extern PFNGLGETUNIFORMINDICESPROC                              glGetUniformIndices;
        extern PFNGLGETUNIFORMLOCATIONPROC                             glGetUniformLocation;
        extern PFNGLGETUNIFORMSUBROUTINEUIVPROC                        glGetUniformSubroutineuiv;
        extern PFNGLGETUNIFORMDVPROC                                   glGetUniformdv;
        extern PFNGLGETUNIFORMFVPROC                                   glGetUniformfv;
        extern PFNGLGETUNIFORMI64VARBPROC                              glGetUniformi64vARB;
        extern PFNGLGETUNIFORMI64VNVPROC                               glGetUniformi64vNV;
        extern PFNGLGETUNIFORMIVPROC                                   glGetUniformiv;
        extern PFNGLGETUNIFORMUI64VARBPROC                             glGetUniformui64vARB;
        extern PFNGLGETUNIFORMUI64VNVPROC                              glGetUniformui64vNV;
        extern PFNGLGETUNIFORMUIVPROC                                  glGetUniformuiv;
        extern PFNGLGETVERTEXARRAYINDEXED64IVPROC                      glGetVertexArrayIndexed64iv;
        extern PFNGLGETVERTEXARRAYINDEXEDIVPROC                        glGetVertexArrayIndexediv;
        extern PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC                    glGetVertexArrayIntegeri_vEXT;
        extern PFNGLGETVERTEXARRAYINTEGERVEXTPROC                      glGetVertexArrayIntegervEXT;
        extern PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC                    glGetVertexArrayPointeri_vEXT;
        extern PFNGLGETVERTEXARRAYPOINTERVEXTPROC                      glGetVertexArrayPointervEXT;
        extern PFNGLGETVERTEXARRAYIVPROC                               glGetVertexArrayiv;
        extern PFNGLGETVERTEXATTRIBIIVPROC                             glGetVertexAttribIiv;
        extern PFNGLGETVERTEXATTRIBIUIVPROC                            glGetVertexAttribIuiv;
        extern PFNGLGETVERTEXATTRIBLDVPROC                             glGetVertexAttribLdv;
        extern PFNGLGETVERTEXATTRIBLI64VNVPROC                         glGetVertexAttribLi64vNV;
        extern PFNGLGETVERTEXATTRIBLUI64VARBPROC                       glGetVertexAttribLui64vARB;
        extern PFNGLGETVERTEXATTRIBLUI64VNVPROC                        glGetVertexAttribLui64vNV;
        extern PFNGLGETVERTEXATTRIBPOINTERVPROC                        glGetVertexAttribPointerv;
        extern PFNGLGETVERTEXATTRIBDVPROC                              glGetVertexAttribdv;
        extern PFNGLGETVERTEXATTRIBFVPROC                              glGetVertexAttribfv;
        extern PFNGLGETVERTEXATTRIBIVPROC                              glGetVertexAttribiv;
        extern PFNGLGETVKPROCADDRNVPROC                                glGetVkProcAddrNV;
        extern PFNGLGETNCOMPRESSEDTEXIMAGEPROC                         glGetnCompressedTexImage;
        extern PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC                      glGetnCompressedTexImageARB;
        extern PFNGLGETNTEXIMAGEPROC                                   glGetnTexImage;
        extern PFNGLGETNTEXIMAGEARBPROC                                glGetnTexImageARB;
        extern PFNGLGETNUNIFORMDVPROC                                  glGetnUniformdv;
        extern PFNGLGETNUNIFORMDVARBPROC                               glGetnUniformdvARB;
        extern PFNGLGETNUNIFORMFVPROC                                  glGetnUniformfv;
        extern PFNGLGETNUNIFORMFVARBPROC                               glGetnUniformfvARB;
        extern PFNGLGETNUNIFORMI64VARBPROC                             glGetnUniformi64vARB;
        extern PFNGLGETNUNIFORMIVPROC                                  glGetnUniformiv;
        extern PFNGLGETNUNIFORMIVARBPROC                               glGetnUniformivARB;
        extern PFNGLGETNUNIFORMUI64VARBPROC                            glGetnUniformui64vARB;
        extern PFNGLGETNUNIFORMUIVPROC                                 glGetnUniformuiv;
        extern PFNGLGETNUNIFORMUIVARBPROC                              glGetnUniformuivARB;
        extern PFNGLHINTPROC                                           glHint;
        extern PFNGLINDEXFORMATNVPROC                                  glIndexFormatNV;
        extern PFNGLINSERTEVENTMARKEREXTPROC                           glInsertEventMarkerEXT;
        extern PFNGLINTERPOLATEPATHSNVPROC                             glInterpolatePathsNV;
        extern PFNGLINVALIDATEBUFFERDATAPROC                           glInvalidateBufferData;
        extern PFNGLINVALIDATEBUFFERSUBDATAPROC                        glInvalidateBufferSubData;
        extern PFNGLINVALIDATEFRAMEBUFFERPROC                          glInvalidateFramebuffer;
        extern PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC                 glInvalidateNamedFramebufferData;
        extern PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC              glInvalidateNamedFramebufferSubData;
        extern PFNGLINVALIDATESUBFRAMEBUFFERPROC                       glInvalidateSubFramebuffer;
        extern PFNGLINVALIDATETEXIMAGEPROC                             glInvalidateTexImage;
        extern PFNGLINVALIDATETEXSUBIMAGEPROC                          glInvalidateTexSubImage;
        extern PFNGLISBUFFERPROC                                       glIsBuffer;
        extern PFNGLISBUFFERRESIDENTNVPROC                             glIsBufferResidentNV;
        extern PFNGLISCOMMANDLISTNVPROC                                glIsCommandListNV;
        extern PFNGLISENABLEDPROC                                      glIsEnabled;
        extern PFNGLISENABLEDINDEXEDEXTPROC                            glIsEnabledIndexedEXT;
        extern PFNGLISENABLEDIPROC                                     glIsEnabledi;
        extern PFNGLISFRAMEBUFFERPROC                                  glIsFramebuffer;
        extern PFNGLISIMAGEHANDLERESIDENTARBPROC                       glIsImageHandleResidentARB;
        extern PFNGLISIMAGEHANDLERESIDENTNVPROC                        glIsImageHandleResidentNV;
        extern PFNGLISNAMEDBUFFERRESIDENTNVPROC                        glIsNamedBufferResidentNV;
        extern PFNGLISNAMEDSTRINGARBPROC                               glIsNamedStringARB;
        extern PFNGLISPATHNVPROC                                       glIsPathNV;
        extern PFNGLISPOINTINFILLPATHNVPROC                            glIsPointInFillPathNV;
        extern PFNGLISPOINTINSTROKEPATHNVPROC                          glIsPointInStrokePathNV;
        extern PFNGLISPROGRAMPROC                                      glIsProgram;
        extern PFNGLISPROGRAMPIPELINEPROC                              glIsProgramPipeline;
        extern PFNGLISQUERYPROC                                        glIsQuery;
        extern PFNGLISRENDERBUFFERPROC                                 glIsRenderbuffer;
        extern PFNGLISSAMPLERPROC                                      glIsSampler;
        extern PFNGLISSHADERPROC                                       glIsShader;
        extern PFNGLISSTATENVPROC                                      glIsStateNV;
        extern PFNGLISSYNCPROC                                         glIsSync;
        extern PFNGLISTEXTUREPROC                                      glIsTexture;
        extern PFNGLISTEXTUREHANDLERESIDENTARBPROC                     glIsTextureHandleResidentARB;
        extern PFNGLISTEXTUREHANDLERESIDENTNVPROC                      glIsTextureHandleResidentNV;
        extern PFNGLISTRANSFORMFEEDBACKPROC                            glIsTransformFeedback;
        extern PFNGLISVERTEXARRAYPROC                                  glIsVertexArray;
        extern PFNGLLABELOBJECTEXTPROC                                 glLabelObjectEXT;
        extern PFNGLLINEWIDTHPROC                                      glLineWidth;
        extern PFNGLLINKPROGRAMPROC                                    glLinkProgram;
        extern PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC                 glListDrawCommandsStatesClientNV;
        extern PFNGLLOGICOPPROC                                        glLogicOp;
        extern PFNGLMAKEBUFFERNONRESIDENTNVPROC                        glMakeBufferNonResidentNV;
        extern PFNGLMAKEBUFFERRESIDENTNVPROC                           glMakeBufferResidentNV;
        extern PFNGLMAKEIMAGEHANDLENONRESIDENTARBPROC                  glMakeImageHandleNonResidentARB;
        extern PFNGLMAKEIMAGEHANDLENONRESIDENTNVPROC                   glMakeImageHandleNonResidentNV;
        extern PFNGLMAKEIMAGEHANDLERESIDENTARBPROC                     glMakeImageHandleResidentARB;
        extern PFNGLMAKEIMAGEHANDLERESIDENTNVPROC                      glMakeImageHandleResidentNV;
        extern PFNGLMAKENAMEDBUFFERNONRESIDENTNVPROC                   glMakeNamedBufferNonResidentNV;
        extern PFNGLMAKENAMEDBUFFERRESIDENTNVPROC                      glMakeNamedBufferResidentNV;
        extern PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC                glMakeTextureHandleNonResidentARB;
        extern PFNGLMAKETEXTUREHANDLENONRESIDENTNVPROC                 glMakeTextureHandleNonResidentNV;
        extern PFNGLMAKETEXTUREHANDLERESIDENTARBPROC                   glMakeTextureHandleResidentARB;
        extern PFNGLMAKETEXTUREHANDLERESIDENTNVPROC                    glMakeTextureHandleResidentNV;
        extern PFNGLMAPBUFFERPROC                                      glMapBuffer;
        extern PFNGLMAPBUFFERRANGEPROC                                 glMapBufferRange;
        extern PFNGLMAPNAMEDBUFFERPROC                                 glMapNamedBuffer;
        extern PFNGLMAPNAMEDBUFFEREXTPROC                              glMapNamedBufferEXT;
        extern PFNGLMAPNAMEDBUFFERRANGEPROC                            glMapNamedBufferRange;
        extern PFNGLMAPNAMEDBUFFERRANGEEXTPROC                         glMapNamedBufferRangeEXT;
        extern PFNGLMATRIXFRUSTUMEXTPROC                               glMatrixFrustumEXT;
        extern PFNGLMATRIXLOAD3X2FNVPROC                               glMatrixLoad3x2fNV;
        extern PFNGLMATRIXLOAD3X3FNVPROC                               glMatrixLoad3x3fNV;
        extern PFNGLMATRIXLOADIDENTITYEXTPROC                          glMatrixLoadIdentityEXT;
        extern PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC                      glMatrixLoadTranspose3x3fNV;
        extern PFNGLMATRIXLOADTRANSPOSEDEXTPROC                        glMatrixLoadTransposedEXT;
        extern PFNGLMATRIXLOADTRANSPOSEFEXTPROC                        glMatrixLoadTransposefEXT;
        extern PFNGLMATRIXLOADDEXTPROC                                 glMatrixLoaddEXT;
        extern PFNGLMATRIXLOADFEXTPROC                                 glMatrixLoadfEXT;
        extern PFNGLMATRIXMULT3X2FNVPROC                               glMatrixMult3x2fNV;
        extern PFNGLMATRIXMULT3X3FNVPROC                               glMatrixMult3x3fNV;
        extern PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC                      glMatrixMultTranspose3x3fNV;
        extern PFNGLMATRIXMULTTRANSPOSEDEXTPROC                        glMatrixMultTransposedEXT;
        extern PFNGLMATRIXMULTTRANSPOSEFEXTPROC                        glMatrixMultTransposefEXT;
        extern PFNGLMATRIXMULTDEXTPROC                                 glMatrixMultdEXT;
        extern PFNGLMATRIXMULTFEXTPROC                                 glMatrixMultfEXT;
        extern PFNGLMATRIXORTHOEXTPROC                                 glMatrixOrthoEXT;
        extern PFNGLMATRIXPOPEXTPROC                                   glMatrixPopEXT;
        extern PFNGLMATRIXPUSHEXTPROC                                  glMatrixPushEXT;
        extern PFNGLMATRIXROTATEDEXTPROC                               glMatrixRotatedEXT;
        extern PFNGLMATRIXROTATEFEXTPROC                               glMatrixRotatefEXT;
        extern PFNGLMATRIXSCALEDEXTPROC                                glMatrixScaledEXT;
        extern PFNGLMATRIXSCALEFEXTPROC                                glMatrixScalefEXT;
        extern PFNGLMATRIXTRANSLATEDEXTPROC                            glMatrixTranslatedEXT;
        extern PFNGLMATRIXTRANSLATEFEXTPROC                            glMatrixTranslatefEXT;
        extern PFNGLMAXSHADERCOMPILERTHREADSARBPROC                    glMaxShaderCompilerThreadsARB;
        extern PFNGLMAXSHADERCOMPILERTHREADSKHRPROC                    glMaxShaderCompilerThreadsKHR;
        extern PFNGLMEMORYBARRIERPROC                                  glMemoryBarrier;
        extern PFNGLMEMORYBARRIERBYREGIONPROC                          glMemoryBarrierByRegion;
        extern PFNGLMINSAMPLESHADINGPROC                               glMinSampleShading;
        extern PFNGLMINSAMPLESHADINGARBPROC                            glMinSampleShadingARB;
        extern PFNGLMULTIDRAWARRAYSPROC                                glMultiDrawArrays;
        extern PFNGLMULTIDRAWARRAYSINDIRECTPROC                        glMultiDrawArraysIndirect;
        extern PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSCOUNTNVPROC         glMultiDrawArraysIndirectBindlessCountNV;
        extern PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSNVPROC              glMultiDrawArraysIndirectBindlessNV;
        extern PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC                   glMultiDrawArraysIndirectCount;
        extern PFNGLMULTIDRAWARRAYSINDIRECTCOUNTARBPROC                glMultiDrawArraysIndirectCountARB;
        extern PFNGLMULTIDRAWELEMENTSPROC                              glMultiDrawElements;
        extern PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC                    glMultiDrawElementsBaseVertex;
        extern PFNGLMULTIDRAWELEMENTSINDIRECTPROC                      glMultiDrawElementsIndirect;
        extern PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSCOUNTNVPROC       glMultiDrawElementsIndirectBindlessCountNV;
        extern PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSNVPROC            glMultiDrawElementsIndirectBindlessNV;
        extern PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC                 glMultiDrawElementsIndirectCount;
        extern PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTARBPROC              glMultiDrawElementsIndirectCountARB;
        extern PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTNVPROC              glMultiDrawMeshTasksIndirectCountNV;
        extern PFNGLMULTIDRAWMESHTASKSINDIRECTNVPROC                   glMultiDrawMeshTasksIndirectNV;
        extern PFNGLMULTITEXBUFFEREXTPROC                              glMultiTexBufferEXT;
        extern PFNGLMULTITEXCOORDPOINTEREXTPROC                        glMultiTexCoordPointerEXT;
        extern PFNGLMULTITEXENVFEXTPROC                                glMultiTexEnvfEXT;
        extern PFNGLMULTITEXENVFVEXTPROC                               glMultiTexEnvfvEXT;
        extern PFNGLMULTITEXENVIEXTPROC                                glMultiTexEnviEXT;
        extern PFNGLMULTITEXENVIVEXTPROC                               glMultiTexEnvivEXT;
        extern PFNGLMULTITEXGENDEXTPROC                                glMultiTexGendEXT;
        extern PFNGLMULTITEXGENDVEXTPROC                               glMultiTexGendvEXT;
        extern PFNGLMULTITEXGENFEXTPROC                                glMultiTexGenfEXT;
        extern PFNGLMULTITEXGENFVEXTPROC                               glMultiTexGenfvEXT;
        extern PFNGLMULTITEXGENIEXTPROC                                glMultiTexGeniEXT;
        extern PFNGLMULTITEXGENIVEXTPROC                               glMultiTexGenivEXT;
        extern PFNGLMULTITEXIMAGE1DEXTPROC                             glMultiTexImage1DEXT;
        extern PFNGLMULTITEXIMAGE2DEXTPROC                             glMultiTexImage2DEXT;
        extern PFNGLMULTITEXIMAGE3DEXTPROC                             glMultiTexImage3DEXT;
        extern PFNGLMULTITEXPARAMETERIIVEXTPROC                        glMultiTexParameterIivEXT;
        extern PFNGLMULTITEXPARAMETERIUIVEXTPROC                       glMultiTexParameterIuivEXT;
        extern PFNGLMULTITEXPARAMETERFEXTPROC                          glMultiTexParameterfEXT;
        extern PFNGLMULTITEXPARAMETERFVEXTPROC                         glMultiTexParameterfvEXT;
        extern PFNGLMULTITEXPARAMETERIEXTPROC                          glMultiTexParameteriEXT;
        extern PFNGLMULTITEXPARAMETERIVEXTPROC                         glMultiTexParameterivEXT;
        extern PFNGLMULTITEXRENDERBUFFEREXTPROC                        glMultiTexRenderbufferEXT;
        extern PFNGLMULTITEXSUBIMAGE1DEXTPROC                          glMultiTexSubImage1DEXT;
        extern PFNGLMULTITEXSUBIMAGE2DEXTPROC                          glMultiTexSubImage2DEXT;
        extern PFNGLMULTITEXSUBIMAGE3DEXTPROC                          glMultiTexSubImage3DEXT;
        extern PFNGLNAMEDBUFFERATTACHMEMORYNVPROC                      glNamedBufferAttachMemoryNV;
        extern PFNGLNAMEDBUFFERDATAPROC                                glNamedBufferData;
        extern PFNGLNAMEDBUFFERDATAEXTPROC                             glNamedBufferDataEXT;
        extern PFNGLNAMEDBUFFERPAGECOMMITMENTARBPROC                   glNamedBufferPageCommitmentARB;
        extern PFNGLNAMEDBUFFERPAGECOMMITMENTEXTPROC                   glNamedBufferPageCommitmentEXT;
        extern PFNGLNAMEDBUFFERSTORAGEPROC                             glNamedBufferStorage;
        extern PFNGLNAMEDBUFFERSTORAGEEXTPROC                          glNamedBufferStorageEXT;
        extern PFNGLNAMEDBUFFERSUBDATAPROC                             glNamedBufferSubData;
        extern PFNGLNAMEDBUFFERSUBDATAEXTPROC                          glNamedBufferSubDataEXT;
        extern PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC                      glNamedCopyBufferSubDataEXT;
        extern PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC                     glNamedFramebufferDrawBuffer;
        extern PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC                    glNamedFramebufferDrawBuffers;
        extern PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC                     glNamedFramebufferParameteri;
        extern PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC                  glNamedFramebufferParameteriEXT;
        extern PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC                     glNamedFramebufferReadBuffer;
        extern PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC                   glNamedFramebufferRenderbuffer;
        extern PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC                glNamedFramebufferRenderbufferEXT;
        extern PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVARBPROC           glNamedFramebufferSampleLocationsfvARB;
        extern PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNVPROC            glNamedFramebufferSampleLocationsfvNV;
        extern PFNGLNAMEDFRAMEBUFFERTEXTUREPROC                        glNamedFramebufferTexture;
        extern PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC                   glNamedFramebufferTexture1DEXT;
        extern PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC                   glNamedFramebufferTexture2DEXT;
        extern PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC                   glNamedFramebufferTexture3DEXT;
        extern PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC                     glNamedFramebufferTextureEXT;
        extern PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC                 glNamedFramebufferTextureFaceEXT;
        extern PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC                   glNamedFramebufferTextureLayer;
        extern PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC                glNamedFramebufferTextureLayerEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC                glNamedProgramLocalParameter4dEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC               glNamedProgramLocalParameter4dvEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC                glNamedProgramLocalParameter4fEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC               glNamedProgramLocalParameter4fvEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC               glNamedProgramLocalParameterI4iEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC              glNamedProgramLocalParameterI4ivEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC              glNamedProgramLocalParameterI4uiEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC             glNamedProgramLocalParameterI4uivEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC              glNamedProgramLocalParameters4fvEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC             glNamedProgramLocalParametersI4ivEXT;
        extern PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC            glNamedProgramLocalParametersI4uivEXT;
        extern PFNGLNAMEDPROGRAMSTRINGEXTPROC                          glNamedProgramStringEXT;
        extern PFNGLNAMEDRENDERBUFFERSTORAGEPROC                       glNamedRenderbufferStorage;
        extern PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC                    glNamedRenderbufferStorageEXT;
        extern PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC            glNamedRenderbufferStorageMultisample;
        extern PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC glNamedRenderbufferStorageMultisampleAdvancedAMD;
        extern PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC glNamedRenderbufferStorageMultisampleCoverageEXT;
        extern PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC         glNamedRenderbufferStorageMultisampleEXT;
        extern PFNGLNAMEDSTRINGARBPROC                                 glNamedStringARB;
        extern PFNGLNORMALFORMATNVPROC                                 glNormalFormatNV;
        extern PFNGLOBJECTLABELPROC                                    glObjectLabel;
        extern PFNGLOBJECTPTRLABELPROC                                 glObjectPtrLabel;
        extern PFNGLPATCHPARAMETERFVPROC                               glPatchParameterfv;
        extern PFNGLPATCHPARAMETERIPROC                                glPatchParameteri;
        extern PFNGLPATHCOMMANDSNVPROC                                 glPathCommandsNV;
        extern PFNGLPATHCOORDSNVPROC                                   glPathCoordsNV;
        extern PFNGLPATHCOVERDEPTHFUNCNVPROC                           glPathCoverDepthFuncNV;
        extern PFNGLPATHDASHARRAYNVPROC                                glPathDashArrayNV;
        extern PFNGLPATHGLYPHINDEXARRAYNVPROC                          glPathGlyphIndexArrayNV;
        extern PFNGLPATHGLYPHINDEXRANGENVPROC                          glPathGlyphIndexRangeNV;
        extern PFNGLPATHGLYPHRANGENVPROC                               glPathGlyphRangeNV;
        extern PFNGLPATHGLYPHSNVPROC                                   glPathGlyphsNV;
        extern PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC                    glPathMemoryGlyphIndexArrayNV;
        extern PFNGLPATHPARAMETERFNVPROC                               glPathParameterfNV;
        extern PFNGLPATHPARAMETERFVNVPROC                              glPathParameterfvNV;
        extern PFNGLPATHPARAMETERINVPROC                               glPathParameteriNV;
        extern PFNGLPATHPARAMETERIVNVPROC                              glPathParameterivNV;
        extern PFNGLPATHSTENCILDEPTHOFFSETNVPROC                       glPathStencilDepthOffsetNV;
        extern PFNGLPATHSTENCILFUNCNVPROC                              glPathStencilFuncNV;
        extern PFNGLPATHSTRINGNVPROC                                   glPathStringNV;
        extern PFNGLPATHSUBCOMMANDSNVPROC                              glPathSubCommandsNV;
        extern PFNGLPATHSUBCOORDSNVPROC                                glPathSubCoordsNV;
        extern PFNGLPAUSETRANSFORMFEEDBACKPROC                         glPauseTransformFeedback;
        extern PFNGLPIXELSTOREFPROC                                    glPixelStoref;
        extern PFNGLPIXELSTOREIPROC                                    glPixelStorei;
        extern PFNGLPOINTALONGPATHNVPROC                               glPointAlongPathNV;
        extern PFNGLPOINTPARAMETERFPROC                                glPointParameterf;
        extern PFNGLPOINTPARAMETERFVPROC                               glPointParameterfv;
        extern PFNGLPOINTPARAMETERIPROC                                glPointParameteri;
        extern PFNGLPOINTPARAMETERIVPROC                               glPointParameteriv;
        extern PFNGLPOINTSIZEPROC                                      glPointSize;
        extern PFNGLPOLYGONMODEPROC                                    glPolygonMode;
        extern PFNGLPOLYGONOFFSETPROC                                  glPolygonOffset;
        extern PFNGLPOLYGONOFFSETCLAMPPROC                             glPolygonOffsetClamp;
        extern PFNGLPOLYGONOFFSETCLAMPEXTPROC                          glPolygonOffsetClampEXT;
        extern PFNGLPOPDEBUGGROUPPROC                                  glPopDebugGroup;
        extern PFNGLPOPGROUPMARKEREXTPROC                              glPopGroupMarkerEXT;
        extern PFNGLPRIMITIVEBOUNDINGBOXARBPROC                        glPrimitiveBoundingBoxARB;
        extern PFNGLPRIMITIVERESTARTINDEXPROC                          glPrimitiveRestartIndex;
        extern PFNGLPROGRAMBINARYPROC                                  glProgramBinary;
        extern PFNGLPROGRAMPARAMETERIPROC                              glProgramParameteri;
        extern PFNGLPROGRAMPARAMETERIARBPROC                           glProgramParameteriARB;
        extern PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC                  glProgramPathFragmentInputGenNV;
        extern PFNGLPROGRAMUNIFORM1DPROC                               glProgramUniform1d;
        extern PFNGLPROGRAMUNIFORM1DEXTPROC                            glProgramUniform1dEXT;
        extern PFNGLPROGRAMUNIFORM1DVPROC                              glProgramUniform1dv;
        extern PFNGLPROGRAMUNIFORM1DVEXTPROC                           glProgramUniform1dvEXT;
        extern PFNGLPROGRAMUNIFORM1FPROC                               glProgramUniform1f;
        extern PFNGLPROGRAMUNIFORM1FEXTPROC                            glProgramUniform1fEXT;
        extern PFNGLPROGRAMUNIFORM1FVPROC                              glProgramUniform1fv;
        extern PFNGLPROGRAMUNIFORM1FVEXTPROC                           glProgramUniform1fvEXT;
        extern PFNGLPROGRAMUNIFORM1IPROC                               glProgramUniform1i;
        extern PFNGLPROGRAMUNIFORM1I64ARBPROC                          glProgramUniform1i64ARB;
        extern PFNGLPROGRAMUNIFORM1I64NVPROC                           glProgramUniform1i64NV;
        extern PFNGLPROGRAMUNIFORM1I64VARBPROC                         glProgramUniform1i64vARB;
        extern PFNGLPROGRAMUNIFORM1I64VNVPROC                          glProgramUniform1i64vNV;
        extern PFNGLPROGRAMUNIFORM1IEXTPROC                            glProgramUniform1iEXT;
        extern PFNGLPROGRAMUNIFORM1IVPROC                              glProgramUniform1iv;
        extern PFNGLPROGRAMUNIFORM1IVEXTPROC                           glProgramUniform1ivEXT;
        extern PFNGLPROGRAMUNIFORM1UIPROC                              glProgramUniform1ui;
        extern PFNGLPROGRAMUNIFORM1UI64ARBPROC                         glProgramUniform1ui64ARB;
        extern PFNGLPROGRAMUNIFORM1UI64NVPROC                          glProgramUniform1ui64NV;
        extern PFNGLPROGRAMUNIFORM1UI64VARBPROC                        glProgramUniform1ui64vARB;
        extern PFNGLPROGRAMUNIFORM1UI64VNVPROC                         glProgramUniform1ui64vNV;
        extern PFNGLPROGRAMUNIFORM1UIEXTPROC                           glProgramUniform1uiEXT;
        extern PFNGLPROGRAMUNIFORM1UIVPROC                             glProgramUniform1uiv;
        extern PFNGLPROGRAMUNIFORM1UIVEXTPROC                          glProgramUniform1uivEXT;
        extern PFNGLPROGRAMUNIFORM2DPROC                               glProgramUniform2d;
        extern PFNGLPROGRAMUNIFORM2DEXTPROC                            glProgramUniform2dEXT;
        extern PFNGLPROGRAMUNIFORM2DVPROC                              glProgramUniform2dv;
        extern PFNGLPROGRAMUNIFORM2DVEXTPROC                           glProgramUniform2dvEXT;
        extern PFNGLPROGRAMUNIFORM2FPROC                               glProgramUniform2f;
        extern PFNGLPROGRAMUNIFORM2FEXTPROC                            glProgramUniform2fEXT;
        extern PFNGLPROGRAMUNIFORM2FVPROC                              glProgramUniform2fv;
        extern PFNGLPROGRAMUNIFORM2FVEXTPROC                           glProgramUniform2fvEXT;
        extern PFNGLPROGRAMUNIFORM2IPROC                               glProgramUniform2i;
        extern PFNGLPROGRAMUNIFORM2I64ARBPROC                          glProgramUniform2i64ARB;
        extern PFNGLPROGRAMUNIFORM2I64NVPROC                           glProgramUniform2i64NV;
        extern PFNGLPROGRAMUNIFORM2I64VARBPROC                         glProgramUniform2i64vARB;
        extern PFNGLPROGRAMUNIFORM2I64VNVPROC                          glProgramUniform2i64vNV;
        extern PFNGLPROGRAMUNIFORM2IEXTPROC                            glProgramUniform2iEXT;
        extern PFNGLPROGRAMUNIFORM2IVPROC                              glProgramUniform2iv;
        extern PFNGLPROGRAMUNIFORM2IVEXTPROC                           glProgramUniform2ivEXT;
        extern PFNGLPROGRAMUNIFORM2UIPROC                              glProgramUniform2ui;
        extern PFNGLPROGRAMUNIFORM2UI64ARBPROC                         glProgramUniform2ui64ARB;
        extern PFNGLPROGRAMUNIFORM2UI64NVPROC                          glProgramUniform2ui64NV;
        extern PFNGLPROGRAMUNIFORM2UI64VARBPROC                        glProgramUniform2ui64vARB;
        extern PFNGLPROGRAMUNIFORM2UI64VNVPROC                         glProgramUniform2ui64vNV;
        extern PFNGLPROGRAMUNIFORM2UIEXTPROC                           glProgramUniform2uiEXT;
        extern PFNGLPROGRAMUNIFORM2UIVPROC                             glProgramUniform2uiv;
        extern PFNGLPROGRAMUNIFORM2UIVEXTPROC                          glProgramUniform2uivEXT;
        extern PFNGLPROGRAMUNIFORM3DPROC                               glProgramUniform3d;
        extern PFNGLPROGRAMUNIFORM3DEXTPROC                            glProgramUniform3dEXT;
        extern PFNGLPROGRAMUNIFORM3DVPROC                              glProgramUniform3dv;
        extern PFNGLPROGRAMUNIFORM3DVEXTPROC                           glProgramUniform3dvEXT;
        extern PFNGLPROGRAMUNIFORM3FPROC                               glProgramUniform3f;
        extern PFNGLPROGRAMUNIFORM3FEXTPROC                            glProgramUniform3fEXT;
        extern PFNGLPROGRAMUNIFORM3FVPROC                              glProgramUniform3fv;
        extern PFNGLPROGRAMUNIFORM3FVEXTPROC                           glProgramUniform3fvEXT;
        extern PFNGLPROGRAMUNIFORM3IPROC                               glProgramUniform3i;
        extern PFNGLPROGRAMUNIFORM3I64ARBPROC                          glProgramUniform3i64ARB;
        extern PFNGLPROGRAMUNIFORM3I64NVPROC                           glProgramUniform3i64NV;
        extern PFNGLPROGRAMUNIFORM3I64VARBPROC                         glProgramUniform3i64vARB;
        extern PFNGLPROGRAMUNIFORM3I64VNVPROC                          glProgramUniform3i64vNV;
        extern PFNGLPROGRAMUNIFORM3IEXTPROC                            glProgramUniform3iEXT;
        extern PFNGLPROGRAMUNIFORM3IVPROC                              glProgramUniform3iv;
        extern PFNGLPROGRAMUNIFORM3IVEXTPROC                           glProgramUniform3ivEXT;
        extern PFNGLPROGRAMUNIFORM3UIPROC                              glProgramUniform3ui;
        extern PFNGLPROGRAMUNIFORM3UI64ARBPROC                         glProgramUniform3ui64ARB;
        extern PFNGLPROGRAMUNIFORM3UI64NVPROC                          glProgramUniform3ui64NV;
        extern PFNGLPROGRAMUNIFORM3UI64VARBPROC                        glProgramUniform3ui64vARB;
        extern PFNGLPROGRAMUNIFORM3UI64VNVPROC                         glProgramUniform3ui64vNV;
        extern PFNGLPROGRAMUNIFORM3UIEXTPROC                           glProgramUniform3uiEXT;
        extern PFNGLPROGRAMUNIFORM3UIVPROC                             glProgramUniform3uiv;
        extern PFNGLPROGRAMUNIFORM3UIVEXTPROC                          glProgramUniform3uivEXT;
        extern PFNGLPROGRAMUNIFORM4DPROC                               glProgramUniform4d;
        extern PFNGLPROGRAMUNIFORM4DEXTPROC                            glProgramUniform4dEXT;
        extern PFNGLPROGRAMUNIFORM4DVPROC                              glProgramUniform4dv;
        extern PFNGLPROGRAMUNIFORM4DVEXTPROC                           glProgramUniform4dvEXT;
        extern PFNGLPROGRAMUNIFORM4FPROC                               glProgramUniform4f;
        extern PFNGLPROGRAMUNIFORM4FEXTPROC                            glProgramUniform4fEXT;
        extern PFNGLPROGRAMUNIFORM4FVPROC                              glProgramUniform4fv;
        extern PFNGLPROGRAMUNIFORM4FVEXTPROC                           glProgramUniform4fvEXT;
        extern PFNGLPROGRAMUNIFORM4IPROC                               glProgramUniform4i;
        extern PFNGLPROGRAMUNIFORM4I64ARBPROC                          glProgramUniform4i64ARB;
        extern PFNGLPROGRAMUNIFORM4I64NVPROC                           glProgramUniform4i64NV;
        extern PFNGLPROGRAMUNIFORM4I64VARBPROC                         glProgramUniform4i64vARB;
        extern PFNGLPROGRAMUNIFORM4I64VNVPROC                          glProgramUniform4i64vNV;
        extern PFNGLPROGRAMUNIFORM4IEXTPROC                            glProgramUniform4iEXT;
        extern PFNGLPROGRAMUNIFORM4IVPROC                              glProgramUniform4iv;
        extern PFNGLPROGRAMUNIFORM4IVEXTPROC                           glProgramUniform4ivEXT;
        extern PFNGLPROGRAMUNIFORM4UIPROC                              glProgramUniform4ui;
        extern PFNGLPROGRAMUNIFORM4UI64ARBPROC                         glProgramUniform4ui64ARB;
        extern PFNGLPROGRAMUNIFORM4UI64NVPROC                          glProgramUniform4ui64NV;
        extern PFNGLPROGRAMUNIFORM4UI64VARBPROC                        glProgramUniform4ui64vARB;
        extern PFNGLPROGRAMUNIFORM4UI64VNVPROC                         glProgramUniform4ui64vNV;
        extern PFNGLPROGRAMUNIFORM4UIEXTPROC                           glProgramUniform4uiEXT;
        extern PFNGLPROGRAMUNIFORM4UIVPROC                             glProgramUniform4uiv;
        extern PFNGLPROGRAMUNIFORM4UIVEXTPROC                          glProgramUniform4uivEXT;
        extern PFNGLPROGRAMUNIFORMHANDLEUI64ARBPROC                    glProgramUniformHandleui64ARB;
        extern PFNGLPROGRAMUNIFORMHANDLEUI64NVPROC                     glProgramUniformHandleui64NV;
        extern PFNGLPROGRAMUNIFORMHANDLEUI64VARBPROC                   glProgramUniformHandleui64vARB;
        extern PFNGLPROGRAMUNIFORMHANDLEUI64VNVPROC                    glProgramUniformHandleui64vNV;
        extern PFNGLPROGRAMUNIFORMMATRIX2DVPROC                        glProgramUniformMatrix2dv;
        extern PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC                     glProgramUniformMatrix2dvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX2FVPROC                        glProgramUniformMatrix2fv;
        extern PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC                     glProgramUniformMatrix2fvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC                      glProgramUniformMatrix2x3dv;
        extern PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC                   glProgramUniformMatrix2x3dvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC                      glProgramUniformMatrix2x3fv;
        extern PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC                   glProgramUniformMatrix2x3fvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC                      glProgramUniformMatrix2x4dv;
        extern PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC                   glProgramUniformMatrix2x4dvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC                      glProgramUniformMatrix2x4fv;
        extern PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC                   glProgramUniformMatrix2x4fvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX3DVPROC                        glProgramUniformMatrix3dv;
        extern PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC                     glProgramUniformMatrix3dvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX3FVPROC                        glProgramUniformMatrix3fv;
        extern PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC                     glProgramUniformMatrix3fvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC                      glProgramUniformMatrix3x2dv;
        extern PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC                   glProgramUniformMatrix3x2dvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC                      glProgramUniformMatrix3x2fv;
        extern PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC                   glProgramUniformMatrix3x2fvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC                      glProgramUniformMatrix3x4dv;
        extern PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC                   glProgramUniformMatrix3x4dvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC                      glProgramUniformMatrix3x4fv;
        extern PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC                   glProgramUniformMatrix3x4fvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX4DVPROC                        glProgramUniformMatrix4dv;
        extern PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC                     glProgramUniformMatrix4dvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX4FVPROC                        glProgramUniformMatrix4fv;
        extern PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC                     glProgramUniformMatrix4fvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC                      glProgramUniformMatrix4x2dv;
        extern PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC                   glProgramUniformMatrix4x2dvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC                      glProgramUniformMatrix4x2fv;
        extern PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC                   glProgramUniformMatrix4x2fvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC                      glProgramUniformMatrix4x3dv;
        extern PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC                   glProgramUniformMatrix4x3dvEXT;
        extern PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC                      glProgramUniformMatrix4x3fv;
        extern PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC                   glProgramUniformMatrix4x3fvEXT;
        extern PFNGLPROGRAMUNIFORMUI64NVPROC                           glProgramUniformui64NV;
        extern PFNGLPROGRAMUNIFORMUI64VNVPROC                          glProgramUniformui64vNV;
        extern PFNGLPROVOKINGVERTEXPROC                                glProvokingVertex;
        extern PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC                     glPushClientAttribDefaultEXT;
        extern PFNGLPUSHDEBUGGROUPPROC                                 glPushDebugGroup;
        extern PFNGLPUSHGROUPMARKEREXTPROC                             glPushGroupMarkerEXT;
        extern PFNGLQUERYCOUNTERPROC                                   glQueryCounter;
        extern PFNGLRASTERSAMPLESEXTPROC                               glRasterSamplesEXT;
        extern PFNGLREADBUFFERPROC                                     glReadBuffer;
        extern PFNGLREADPIXELSPROC                                     glReadPixels;
        extern PFNGLREADNPIXELSPROC                                    glReadnPixels;
        extern PFNGLREADNPIXELSARBPROC                                 glReadnPixelsARB;
        extern PFNGLRELEASESHADERCOMPILERPROC                          glReleaseShaderCompiler;
        extern PFNGLRENDERBUFFERSTORAGEPROC                            glRenderbufferStorage;
        extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC                 glRenderbufferStorageMultisample;
        extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC      glRenderbufferStorageMultisampleAdvancedAMD;
        extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLECOVERAGENVPROC       glRenderbufferStorageMultisampleCoverageNV;
        extern PFNGLRESETMEMORYOBJECTPARAMETERNVPROC                   glResetMemoryObjectParameterNV;
        extern PFNGLRESOLVEDEPTHVALUESNVPROC                           glResolveDepthValuesNV;
        extern PFNGLRESUMETRANSFORMFEEDBACKPROC                        glResumeTransformFeedback;
        extern PFNGLSAMPLECOVERAGEPROC                                 glSampleCoverage;
        extern PFNGLSAMPLEMASKIPROC                                    glSampleMaski;
        extern PFNGLSAMPLERPARAMETERIIVPROC                            glSamplerParameterIiv;
        extern PFNGLSAMPLERPARAMETERIUIVPROC                           glSamplerParameterIuiv;
        extern PFNGLSAMPLERPARAMETERFPROC                              glSamplerParameterf;
        extern PFNGLSAMPLERPARAMETERFVPROC                             glSamplerParameterfv;
        extern PFNGLSAMPLERPARAMETERIPROC                              glSamplerParameteri;
        extern PFNGLSAMPLERPARAMETERIVPROC                             glSamplerParameteriv;
        extern PFNGLSCISSORPROC                                        glScissor;
        extern PFNGLSCISSORARRAYVPROC                                  glScissorArrayv;
        extern PFNGLSCISSOREXCLUSIVEARRAYVNVPROC                       glScissorExclusiveArrayvNV;
        extern PFNGLSCISSOREXCLUSIVENVPROC                             glScissorExclusiveNV;
        extern PFNGLSCISSORINDEXEDPROC                                 glScissorIndexed;
        extern PFNGLSCISSORINDEXEDVPROC                                glScissorIndexedv;
        extern PFNGLSECONDARYCOLORFORMATNVPROC                         glSecondaryColorFormatNV;
        extern PFNGLSELECTPERFMONITORCOUNTERSAMDPROC                   glSelectPerfMonitorCountersAMD;
        extern PFNGLSHADERBINARYPROC                                   glShaderBinary;
        extern PFNGLSHADERSOURCEPROC                                   glShaderSource;
        extern PFNGLSHADERSTORAGEBLOCKBINDINGPROC                      glShaderStorageBlockBinding;
        extern PFNGLSHADINGRATEIMAGEBARRIERNVPROC                      glShadingRateImageBarrierNV;
        extern PFNGLSHADINGRATEIMAGEPALETTENVPROC                      glShadingRateImagePaletteNV;
        extern PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC                 glShadingRateSampleOrderCustomNV;
        extern PFNGLSHADINGRATESAMPLEORDERNVPROC                       glShadingRateSampleOrderNV;
        extern PFNGLSIGNALVKFENCENVPROC                                glSignalVkFenceNV;
        extern PFNGLSIGNALVKSEMAPHORENVPROC                            glSignalVkSemaphoreNV;
        extern PFNGLSPECIALIZESHADERPROC                               glSpecializeShader;
        extern PFNGLSPECIALIZESHADERARBPROC                            glSpecializeShaderARB;
        extern PFNGLSTATECAPTURENVPROC                                 glStateCaptureNV;
        extern PFNGLSTENCILFILLPATHINSTANCEDNVPROC                     glStencilFillPathInstancedNV;
        extern PFNGLSTENCILFILLPATHNVPROC                              glStencilFillPathNV;
        extern PFNGLSTENCILFUNCPROC                                    glStencilFunc;
        extern PFNGLSTENCILFUNCSEPARATEPROC                            glStencilFuncSeparate;
        extern PFNGLSTENCILMASKPROC                                    glStencilMask;
        extern PFNGLSTENCILMASKSEPARATEPROC                            glStencilMaskSeparate;
        extern PFNGLSTENCILOPPROC                                      glStencilOp;
        extern PFNGLSTENCILOPSEPARATEPROC                              glStencilOpSeparate;
        extern PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC                   glStencilStrokePathInstancedNV;
        extern PFNGLSTENCILSTROKEPATHNVPROC                            glStencilStrokePathNV;
        extern PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC            glStencilThenCoverFillPathInstancedNV;
        extern PFNGLSTENCILTHENCOVERFILLPATHNVPROC                     glStencilThenCoverFillPathNV;
        extern PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC          glStencilThenCoverStrokePathInstancedNV;
        extern PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC                   glStencilThenCoverStrokePathNV;
        extern PFNGLSUBPIXELPRECISIONBIASNVPROC                        glSubpixelPrecisionBiasNV;
        extern PFNGLTEXATTACHMEMORYNVPROC                              glTexAttachMemoryNV;
        extern PFNGLTEXBUFFERPROC                                      glTexBuffer;
        extern PFNGLTEXBUFFERARBPROC                                   glTexBufferARB;
        extern PFNGLTEXBUFFERRANGEPROC                                 glTexBufferRange;
        extern PFNGLTEXCOORDFORMATNVPROC                               glTexCoordFormatNV;
        extern PFNGLTEXIMAGE1DPROC                                     glTexImage1D;
        extern PFNGLTEXIMAGE2DPROC                                     glTexImage2D;
        extern PFNGLTEXIMAGE2DMULTISAMPLEPROC                          glTexImage2DMultisample;
        extern PFNGLTEXIMAGE3DPROC                                     glTexImage3D;
        extern PFNGLTEXIMAGE3DMULTISAMPLEPROC                          glTexImage3DMultisample;
        extern PFNGLTEXPAGECOMMITMENTARBPROC                           glTexPageCommitmentARB;
        extern PFNGLTEXPARAMETERIIVPROC                                glTexParameterIiv;
        extern PFNGLTEXPARAMETERIUIVPROC                               glTexParameterIuiv;
        extern PFNGLTEXPARAMETERFPROC                                  glTexParameterf;
        extern PFNGLTEXPARAMETERFVPROC                                 glTexParameterfv;
        extern PFNGLTEXPARAMETERIPROC                                  glTexParameteri;
        extern PFNGLTEXPARAMETERIVPROC                                 glTexParameteriv;
        extern PFNGLTEXSTORAGE1DPROC                                   glTexStorage1D;
        extern PFNGLTEXSTORAGE2DPROC                                   glTexStorage2D;
        extern PFNGLTEXSTORAGE2DMULTISAMPLEPROC                        glTexStorage2DMultisample;
        extern PFNGLTEXSTORAGE3DPROC                                   glTexStorage3D;
        extern PFNGLTEXSTORAGE3DMULTISAMPLEPROC                        glTexStorage3DMultisample;
        extern PFNGLTEXSUBIMAGE1DPROC                                  glTexSubImage1D;
        extern PFNGLTEXSUBIMAGE2DPROC                                  glTexSubImage2D;
        extern PFNGLTEXSUBIMAGE3DPROC                                  glTexSubImage3D;
        extern PFNGLTEXTUREATTACHMEMORYNVPROC                          glTextureAttachMemoryNV;
        extern PFNGLTEXTUREBARRIERPROC                                 glTextureBarrier;
        extern PFNGLTEXTUREBARRIERNVPROC                               glTextureBarrierNV;
        extern PFNGLTEXTUREBUFFERPROC                                  glTextureBuffer;
        extern PFNGLTEXTUREBUFFEREXTPROC                               glTextureBufferEXT;
        extern PFNGLTEXTUREBUFFERRANGEPROC                             glTextureBufferRange;
        extern PFNGLTEXTUREBUFFERRANGEEXTPROC                          glTextureBufferRangeEXT;
        extern PFNGLTEXTUREIMAGE1DEXTPROC                              glTextureImage1DEXT;
        extern PFNGLTEXTUREIMAGE2DEXTPROC                              glTextureImage2DEXT;
        extern PFNGLTEXTUREIMAGE3DEXTPROC                              glTextureImage3DEXT;
        extern PFNGLTEXTUREPAGECOMMITMENTEXTPROC                       glTexturePageCommitmentEXT;
        extern PFNGLTEXTUREPARAMETERIIVPROC                            glTextureParameterIiv;
        extern PFNGLTEXTUREPARAMETERIIVEXTPROC                         glTextureParameterIivEXT;
        extern PFNGLTEXTUREPARAMETERIUIVPROC                           glTextureParameterIuiv;
        extern PFNGLTEXTUREPARAMETERIUIVEXTPROC                        glTextureParameterIuivEXT;
        extern PFNGLTEXTUREPARAMETERFPROC                              glTextureParameterf;
        extern PFNGLTEXTUREPARAMETERFEXTPROC                           glTextureParameterfEXT;
        extern PFNGLTEXTUREPARAMETERFVPROC                             glTextureParameterfv;
        extern PFNGLTEXTUREPARAMETERFVEXTPROC                          glTextureParameterfvEXT;
        extern PFNGLTEXTUREPARAMETERIPROC                              glTextureParameteri;
        extern PFNGLTEXTUREPARAMETERIEXTPROC                           glTextureParameteriEXT;
        extern PFNGLTEXTUREPARAMETERIVPROC                             glTextureParameteriv;
        extern PFNGLTEXTUREPARAMETERIVEXTPROC                          glTextureParameterivEXT;
        extern PFNGLTEXTURERENDERBUFFEREXTPROC                         glTextureRenderbufferEXT;
        extern PFNGLTEXTURESTORAGE1DPROC                               glTextureStorage1D;
        extern PFNGLTEXTURESTORAGE1DEXTPROC                            glTextureStorage1DEXT;
        extern PFNGLTEXTURESTORAGE2DPROC                               glTextureStorage2D;
        extern PFNGLTEXTURESTORAGE2DEXTPROC                            glTextureStorage2DEXT;
        extern PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC                    glTextureStorage2DMultisample;
        extern PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC                 glTextureStorage2DMultisampleEXT;
        extern PFNGLTEXTURESTORAGE3DPROC                               glTextureStorage3D;
        extern PFNGLTEXTURESTORAGE3DEXTPROC                            glTextureStorage3DEXT;
        extern PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC                    glTextureStorage3DMultisample;
        extern PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC                 glTextureStorage3DMultisampleEXT;
        extern PFNGLTEXTURESUBIMAGE1DPROC                              glTextureSubImage1D;
        extern PFNGLTEXTURESUBIMAGE1DEXTPROC                           glTextureSubImage1DEXT;
        extern PFNGLTEXTURESUBIMAGE2DPROC                              glTextureSubImage2D;
        extern PFNGLTEXTURESUBIMAGE2DEXTPROC                           glTextureSubImage2DEXT;
        extern PFNGLTEXTURESUBIMAGE3DPROC                              glTextureSubImage3D;
        extern PFNGLTEXTURESUBIMAGE3DEXTPROC                           glTextureSubImage3DEXT;
        extern PFNGLTEXTUREVIEWPROC                                    glTextureView;
        extern PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC                    glTransformFeedbackBufferBase;
        extern PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC                   glTransformFeedbackBufferRange;
        extern PFNGLTRANSFORMFEEDBACKVARYINGSPROC                      glTransformFeedbackVaryings;
        extern PFNGLTRANSFORMPATHNVPROC                                glTransformPathNV;
        extern PFNGLUNIFORM1DPROC                                      glUniform1d;
        extern PFNGLUNIFORM1DVPROC                                     glUniform1dv;
        extern PFNGLUNIFORM1FPROC                                      glUniform1f;
        extern PFNGLUNIFORM1FVPROC                                     glUniform1fv;
        extern PFNGLUNIFORM1IPROC                                      glUniform1i;
        extern PFNGLUNIFORM1I64ARBPROC                                 glUniform1i64ARB;
        extern PFNGLUNIFORM1I64NVPROC                                  glUniform1i64NV;
        extern PFNGLUNIFORM1I64VARBPROC                                glUniform1i64vARB;
        extern PFNGLUNIFORM1I64VNVPROC                                 glUniform1i64vNV;
        extern PFNGLUNIFORM1IVPROC                                     glUniform1iv;
        extern PFNGLUNIFORM1UIPROC                                     glUniform1ui;
        extern PFNGLUNIFORM1UI64ARBPROC                                glUniform1ui64ARB;
        extern PFNGLUNIFORM1UI64NVPROC                                 glUniform1ui64NV;
        extern PFNGLUNIFORM1UI64VARBPROC                               glUniform1ui64vARB;
        extern PFNGLUNIFORM1UI64VNVPROC                                glUniform1ui64vNV;
        extern PFNGLUNIFORM1UIVPROC                                    glUniform1uiv;
        extern PFNGLUNIFORM2DPROC                                      glUniform2d;
        extern PFNGLUNIFORM2DVPROC                                     glUniform2dv;
        extern PFNGLUNIFORM2FPROC                                      glUniform2f;
        extern PFNGLUNIFORM2FVPROC                                     glUniform2fv;
        extern PFNGLUNIFORM2IPROC                                      glUniform2i;
        extern PFNGLUNIFORM2I64ARBPROC                                 glUniform2i64ARB;
        extern PFNGLUNIFORM2I64NVPROC                                  glUniform2i64NV;
        extern PFNGLUNIFORM2I64VARBPROC                                glUniform2i64vARB;
        extern PFNGLUNIFORM2I64VNVPROC                                 glUniform2i64vNV;
        extern PFNGLUNIFORM2IVPROC                                     glUniform2iv;
        extern PFNGLUNIFORM2UIPROC                                     glUniform2ui;
        extern PFNGLUNIFORM2UI64ARBPROC                                glUniform2ui64ARB;
        extern PFNGLUNIFORM2UI64NVPROC                                 glUniform2ui64NV;
        extern PFNGLUNIFORM2UI64VARBPROC                               glUniform2ui64vARB;
        extern PFNGLUNIFORM2UI64VNVPROC                                glUniform2ui64vNV;
        extern PFNGLUNIFORM2UIVPROC                                    glUniform2uiv;
        extern PFNGLUNIFORM3DPROC                                      glUniform3d;
        extern PFNGLUNIFORM3DVPROC                                     glUniform3dv;
        extern PFNGLUNIFORM3FPROC                                      glUniform3f;
        extern PFNGLUNIFORM3FVPROC                                     glUniform3fv;
        extern PFNGLUNIFORM3IPROC                                      glUniform3i;
        extern PFNGLUNIFORM3I64ARBPROC                                 glUniform3i64ARB;
        extern PFNGLUNIFORM3I64NVPROC                                  glUniform3i64NV;
        extern PFNGLUNIFORM3I64VARBPROC                                glUniform3i64vARB;
        extern PFNGLUNIFORM3I64VNVPROC                                 glUniform3i64vNV;
        extern PFNGLUNIFORM3IVPROC                                     glUniform3iv;
        extern PFNGLUNIFORM3UIPROC                                     glUniform3ui;
        extern PFNGLUNIFORM3UI64ARBPROC                                glUniform3ui64ARB;
        extern PFNGLUNIFORM3UI64NVPROC                                 glUniform3ui64NV;
        extern PFNGLUNIFORM3UI64VARBPROC                               glUniform3ui64vARB;
        extern PFNGLUNIFORM3UI64VNVPROC                                glUniform3ui64vNV;
        extern PFNGLUNIFORM3UIVPROC                                    glUniform3uiv;
        extern PFNGLUNIFORM4DPROC                                      glUniform4d;
        extern PFNGLUNIFORM4DVPROC                                     glUniform4dv;
        extern PFNGLUNIFORM4FPROC                                      glUniform4f;
        extern PFNGLUNIFORM4FVPROC                                     glUniform4fv;
        extern PFNGLUNIFORM4IPROC                                      glUniform4i;
        extern PFNGLUNIFORM4I64ARBPROC                                 glUniform4i64ARB;
        extern PFNGLUNIFORM4I64NVPROC                                  glUniform4i64NV;
        extern PFNGLUNIFORM4I64VARBPROC                                glUniform4i64vARB;
        extern PFNGLUNIFORM4I64VNVPROC                                 glUniform4i64vNV;
        extern PFNGLUNIFORM4IVPROC                                     glUniform4iv;
        extern PFNGLUNIFORM4UIPROC                                     glUniform4ui;
        extern PFNGLUNIFORM4UI64ARBPROC                                glUniform4ui64ARB;
        extern PFNGLUNIFORM4UI64NVPROC                                 glUniform4ui64NV;
        extern PFNGLUNIFORM4UI64VARBPROC                               glUniform4ui64vARB;
        extern PFNGLUNIFORM4UI64VNVPROC                                glUniform4ui64vNV;
        extern PFNGLUNIFORM4UIVPROC                                    glUniform4uiv;
        extern PFNGLUNIFORMBLOCKBINDINGPROC                            glUniformBlockBinding;
        extern PFNGLUNIFORMHANDLEUI64ARBPROC                           glUniformHandleui64ARB;
        extern PFNGLUNIFORMHANDLEUI64NVPROC                            glUniformHandleui64NV;
        extern PFNGLUNIFORMHANDLEUI64VARBPROC                          glUniformHandleui64vARB;
        extern PFNGLUNIFORMHANDLEUI64VNVPROC                           glUniformHandleui64vNV;
        extern PFNGLUNIFORMMATRIX2DVPROC                               glUniformMatrix2dv;
        extern PFNGLUNIFORMMATRIX2FVPROC                               glUniformMatrix2fv;
        extern PFNGLUNIFORMMATRIX2X3DVPROC                             glUniformMatrix2x3dv;
        extern PFNGLUNIFORMMATRIX2X3FVPROC                             glUniformMatrix2x3fv;
        extern PFNGLUNIFORMMATRIX2X4DVPROC                             glUniformMatrix2x4dv;
        extern PFNGLUNIFORMMATRIX2X4FVPROC                             glUniformMatrix2x4fv;
        extern PFNGLUNIFORMMATRIX3DVPROC                               glUniformMatrix3dv;
        extern PFNGLUNIFORMMATRIX3FVPROC                               glUniformMatrix3fv;
        extern PFNGLUNIFORMMATRIX3X2DVPROC                             glUniformMatrix3x2dv;
        extern PFNGLUNIFORMMATRIX3X2FVPROC                             glUniformMatrix3x2fv;
        extern PFNGLUNIFORMMATRIX3X4DVPROC                             glUniformMatrix3x4dv;
        extern PFNGLUNIFORMMATRIX3X4FVPROC                             glUniformMatrix3x4fv;
        extern PFNGLUNIFORMMATRIX4DVPROC                               glUniformMatrix4dv;
        extern PFNGLUNIFORMMATRIX4FVPROC                               glUniformMatrix4fv;
        extern PFNGLUNIFORMMATRIX4X2DVPROC                             glUniformMatrix4x2dv;
        extern PFNGLUNIFORMMATRIX4X2FVPROC                             glUniformMatrix4x2fv;
        extern PFNGLUNIFORMMATRIX4X3DVPROC                             glUniformMatrix4x3dv;
        extern PFNGLUNIFORMMATRIX4X3FVPROC                             glUniformMatrix4x3fv;
        extern PFNGLUNIFORMSUBROUTINESUIVPROC                          glUniformSubroutinesuiv;
        extern PFNGLUNIFORMUI64NVPROC                                  glUniformui64NV;
        extern PFNGLUNIFORMUI64VNVPROC                                 glUniformui64vNV;
        extern PFNGLUNMAPBUFFERPROC                                    glUnmapBuffer;
        extern PFNGLUNMAPNAMEDBUFFERPROC                               glUnmapNamedBuffer;
        extern PFNGLUNMAPNAMEDBUFFEREXTPROC                            glUnmapNamedBufferEXT;
        extern PFNGLUSEPROGRAMPROC                                     glUseProgram;
        extern PFNGLUSEPROGRAMSTAGESPROC                               glUseProgramStages;
        extern PFNGLUSESHADERPROGRAMEXTPROC                            glUseShaderProgramEXT;
        extern PFNGLVALIDATEPROGRAMPROC                                glValidateProgram;
        extern PFNGLVALIDATEPROGRAMPIPELINEPROC                        glValidateProgramPipeline;
        extern PFNGLVERTEXARRAYATTRIBBINDINGPROC                       glVertexArrayAttribBinding;
        extern PFNGLVERTEXARRAYATTRIBFORMATPROC                        glVertexArrayAttribFormat;
        extern PFNGLVERTEXARRAYATTRIBIFORMATPROC                       glVertexArrayAttribIFormat;
        extern PFNGLVERTEXARRAYATTRIBLFORMATPROC                       glVertexArrayAttribLFormat;
        extern PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC                 glVertexArrayBindVertexBufferEXT;
        extern PFNGLVERTEXARRAYBINDINGDIVISORPROC                      glVertexArrayBindingDivisor;
        extern PFNGLVERTEXARRAYCOLOROFFSETEXTPROC                      glVertexArrayColorOffsetEXT;
        extern PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC                   glVertexArrayEdgeFlagOffsetEXT;
        extern PFNGLVERTEXARRAYELEMENTBUFFERPROC                       glVertexArrayElementBuffer;
        extern PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC                   glVertexArrayFogCoordOffsetEXT;
        extern PFNGLVERTEXARRAYINDEXOFFSETEXTPROC                      glVertexArrayIndexOffsetEXT;
        extern PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC              glVertexArrayMultiTexCoordOffsetEXT;
        extern PFNGLVERTEXARRAYNORMALOFFSETEXTPROC                     glVertexArrayNormalOffsetEXT;
        extern PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC             glVertexArraySecondaryColorOffsetEXT;
        extern PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC                   glVertexArrayTexCoordOffsetEXT;
        extern PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC              glVertexArrayVertexAttribBindingEXT;
        extern PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC              glVertexArrayVertexAttribDivisorEXT;
        extern PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC               glVertexArrayVertexAttribFormatEXT;
        extern PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC              glVertexArrayVertexAttribIFormatEXT;
        extern PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC              glVertexArrayVertexAttribIOffsetEXT;
        extern PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC              glVertexArrayVertexAttribLFormatEXT;
        extern PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC              glVertexArrayVertexAttribLOffsetEXT;
        extern PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC               glVertexArrayVertexAttribOffsetEXT;
        extern PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC             glVertexArrayVertexBindingDivisorEXT;
        extern PFNGLVERTEXARRAYVERTEXBUFFERPROC                        glVertexArrayVertexBuffer;
        extern PFNGLVERTEXARRAYVERTEXBUFFERSPROC                       glVertexArrayVertexBuffers;
        extern PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC                     glVertexArrayVertexOffsetEXT;
        extern PFNGLVERTEXATTRIB1DPROC                                 glVertexAttrib1d;
        extern PFNGLVERTEXATTRIB1DVPROC                                glVertexAttrib1dv;
        extern PFNGLVERTEXATTRIB1FPROC                                 glVertexAttrib1f;
        extern PFNGLVERTEXATTRIB1FVPROC                                glVertexAttrib1fv;
        extern PFNGLVERTEXATTRIB1SPROC                                 glVertexAttrib1s;
        extern PFNGLVERTEXATTRIB1SVPROC                                glVertexAttrib1sv;
        extern PFNGLVERTEXATTRIB2DPROC                                 glVertexAttrib2d;
        extern PFNGLVERTEXATTRIB2DVPROC                                glVertexAttrib2dv;
        extern PFNGLVERTEXATTRIB2FPROC                                 glVertexAttrib2f;
        extern PFNGLVERTEXATTRIB2FVPROC                                glVertexAttrib2fv;
        extern PFNGLVERTEXATTRIB2SPROC                                 glVertexAttrib2s;
        extern PFNGLVERTEXATTRIB2SVPROC                                glVertexAttrib2sv;
        extern PFNGLVERTEXATTRIB3DPROC                                 glVertexAttrib3d;
        extern PFNGLVERTEXATTRIB3DVPROC                                glVertexAttrib3dv;
        extern PFNGLVERTEXATTRIB3FPROC                                 glVertexAttrib3f;
        extern PFNGLVERTEXATTRIB3FVPROC                                glVertexAttrib3fv;
        extern PFNGLVERTEXATTRIB3SPROC                                 glVertexAttrib3s;
        extern PFNGLVERTEXATTRIB3SVPROC                                glVertexAttrib3sv;
        extern PFNGLVERTEXATTRIB4NBVPROC                               glVertexAttrib4Nbv;
        extern PFNGLVERTEXATTRIB4NIVPROC                               glVertexAttrib4Niv;
        extern PFNGLVERTEXATTRIB4NSVPROC                               glVertexAttrib4Nsv;
        extern PFNGLVERTEXATTRIB4NUBPROC                               glVertexAttrib4Nub;
        extern PFNGLVERTEXATTRIB4NUBVPROC                              glVertexAttrib4Nubv;
        extern PFNGLVERTEXATTRIB4NUIVPROC                              glVertexAttrib4Nuiv;
        extern PFNGLVERTEXATTRIB4NUSVPROC                              glVertexAttrib4Nusv;
        extern PFNGLVERTEXATTRIB4BVPROC                                glVertexAttrib4bv;
        extern PFNGLVERTEXATTRIB4DPROC                                 glVertexAttrib4d;
        extern PFNGLVERTEXATTRIB4DVPROC                                glVertexAttrib4dv;
        extern PFNGLVERTEXATTRIB4FPROC                                 glVertexAttrib4f;
        extern PFNGLVERTEXATTRIB4FVPROC                                glVertexAttrib4fv;
        extern PFNGLVERTEXATTRIB4IVPROC                                glVertexAttrib4iv;
        extern PFNGLVERTEXATTRIB4SPROC                                 glVertexAttrib4s;
        extern PFNGLVERTEXATTRIB4SVPROC                                glVertexAttrib4sv;
        extern PFNGLVERTEXATTRIB4UBVPROC                               glVertexAttrib4ubv;
        extern PFNGLVERTEXATTRIB4UIVPROC                               glVertexAttrib4uiv;
        extern PFNGLVERTEXATTRIB4USVPROC                               glVertexAttrib4usv;
        extern PFNGLVERTEXATTRIBBINDINGPROC                            glVertexAttribBinding;
        extern PFNGLVERTEXATTRIBDIVISORPROC                            glVertexAttribDivisor;
        extern PFNGLVERTEXATTRIBDIVISORARBPROC                         glVertexAttribDivisorARB;
        extern PFNGLVERTEXATTRIBFORMATPROC                             glVertexAttribFormat;
        extern PFNGLVERTEXATTRIBFORMATNVPROC                           glVertexAttribFormatNV;
        extern PFNGLVERTEXATTRIBI1IPROC                                glVertexAttribI1i;
        extern PFNGLVERTEXATTRIBI1IVPROC                               glVertexAttribI1iv;
        extern PFNGLVERTEXATTRIBI1UIPROC                               glVertexAttribI1ui;
        extern PFNGLVERTEXATTRIBI1UIVPROC                              glVertexAttribI1uiv;
        extern PFNGLVERTEXATTRIBI2IPROC                                glVertexAttribI2i;
        extern PFNGLVERTEXATTRIBI2IVPROC                               glVertexAttribI2iv;
        extern PFNGLVERTEXATTRIBI2UIPROC                               glVertexAttribI2ui;
        extern PFNGLVERTEXATTRIBI2UIVPROC                              glVertexAttribI2uiv;
        extern PFNGLVERTEXATTRIBI3IPROC                                glVertexAttribI3i;
        extern PFNGLVERTEXATTRIBI3IVPROC                               glVertexAttribI3iv;
        extern PFNGLVERTEXATTRIBI3UIPROC                               glVertexAttribI3ui;
        extern PFNGLVERTEXATTRIBI3UIVPROC                              glVertexAttribI3uiv;
        extern PFNGLVERTEXATTRIBI4BVPROC                               glVertexAttribI4bv;
        extern PFNGLVERTEXATTRIBI4IPROC                                glVertexAttribI4i;
        extern PFNGLVERTEXATTRIBI4IVPROC                               glVertexAttribI4iv;
        extern PFNGLVERTEXATTRIBI4SVPROC                               glVertexAttribI4sv;
        extern PFNGLVERTEXATTRIBI4UBVPROC                              glVertexAttribI4ubv;
        extern PFNGLVERTEXATTRIBI4UIPROC                               glVertexAttribI4ui;
        extern PFNGLVERTEXATTRIBI4UIVPROC                              glVertexAttribI4uiv;
        extern PFNGLVERTEXATTRIBI4USVPROC                              glVertexAttribI4usv;
        extern PFNGLVERTEXATTRIBIFORMATPROC                            glVertexAttribIFormat;
        extern PFNGLVERTEXATTRIBIFORMATNVPROC                          glVertexAttribIFormatNV;
        extern PFNGLVERTEXATTRIBIPOINTERPROC                           glVertexAttribIPointer;
        extern PFNGLVERTEXATTRIBL1DPROC                                glVertexAttribL1d;
        extern PFNGLVERTEXATTRIBL1DVPROC                               glVertexAttribL1dv;
        extern PFNGLVERTEXATTRIBL1I64NVPROC                            glVertexAttribL1i64NV;
        extern PFNGLVERTEXATTRIBL1I64VNVPROC                           glVertexAttribL1i64vNV;
        extern PFNGLVERTEXATTRIBL1UI64ARBPROC                          glVertexAttribL1ui64ARB;
        extern PFNGLVERTEXATTRIBL1UI64NVPROC                           glVertexAttribL1ui64NV;
        extern PFNGLVERTEXATTRIBL1UI64VARBPROC                         glVertexAttribL1ui64vARB;
        extern PFNGLVERTEXATTRIBL1UI64VNVPROC                          glVertexAttribL1ui64vNV;
        extern PFNGLVERTEXATTRIBL2DPROC                                glVertexAttribL2d;
        extern PFNGLVERTEXATTRIBL2DVPROC                               glVertexAttribL2dv;
        extern PFNGLVERTEXATTRIBL2I64NVPROC                            glVertexAttribL2i64NV;
        extern PFNGLVERTEXATTRIBL2I64VNVPROC                           glVertexAttribL2i64vNV;
        extern PFNGLVERTEXATTRIBL2UI64NVPROC                           glVertexAttribL2ui64NV;
        extern PFNGLVERTEXATTRIBL2UI64VNVPROC                          glVertexAttribL2ui64vNV;
        extern PFNGLVERTEXATTRIBL3DPROC                                glVertexAttribL3d;
        extern PFNGLVERTEXATTRIBL3DVPROC                               glVertexAttribL3dv;
        extern PFNGLVERTEXATTRIBL3I64NVPROC                            glVertexAttribL3i64NV;
        extern PFNGLVERTEXATTRIBL3I64VNVPROC                           glVertexAttribL3i64vNV;
        extern PFNGLVERTEXATTRIBL3UI64NVPROC                           glVertexAttribL3ui64NV;
        extern PFNGLVERTEXATTRIBL3UI64VNVPROC                          glVertexAttribL3ui64vNV;
        extern PFNGLVERTEXATTRIBL4DPROC                                glVertexAttribL4d;
        extern PFNGLVERTEXATTRIBL4DVPROC                               glVertexAttribL4dv;
        extern PFNGLVERTEXATTRIBL4I64NVPROC                            glVertexAttribL4i64NV;
        extern PFNGLVERTEXATTRIBL4I64VNVPROC                           glVertexAttribL4i64vNV;
        extern PFNGLVERTEXATTRIBL4UI64NVPROC                           glVertexAttribL4ui64NV;
        extern PFNGLVERTEXATTRIBL4UI64VNVPROC                          glVertexAttribL4ui64vNV;
        extern PFNGLVERTEXATTRIBLFORMATPROC                            glVertexAttribLFormat;
        extern PFNGLVERTEXATTRIBLFORMATNVPROC                          glVertexAttribLFormatNV;
        extern PFNGLVERTEXATTRIBLPOINTERPROC                           glVertexAttribLPointer;
        extern PFNGLVERTEXATTRIBP1UIPROC                               glVertexAttribP1ui;
        extern PFNGLVERTEXATTRIBP1UIVPROC                              glVertexAttribP1uiv;
        extern PFNGLVERTEXATTRIBP2UIPROC                               glVertexAttribP2ui;
        extern PFNGLVERTEXATTRIBP2UIVPROC                              glVertexAttribP2uiv;
        extern PFNGLVERTEXATTRIBP3UIPROC                               glVertexAttribP3ui;
        extern PFNGLVERTEXATTRIBP3UIVPROC                              glVertexAttribP3uiv;
        extern PFNGLVERTEXATTRIBP4UIPROC                               glVertexAttribP4ui;
        extern PFNGLVERTEXATTRIBP4UIVPROC                              glVertexAttribP4uiv;
        extern PFNGLVERTEXATTRIBPOINTERPROC                            glVertexAttribPointer;
        extern PFNGLVERTEXBINDINGDIVISORPROC                           glVertexBindingDivisor;
        extern PFNGLVERTEXFORMATNVPROC                                 glVertexFormatNV;
        extern PFNGLVIEWPORTPROC                                       glViewport;
        extern PFNGLVIEWPORTARRAYVPROC                                 glViewportArrayv;
        extern PFNGLVIEWPORTINDEXEDFPROC                               glViewportIndexedf;
        extern PFNGLVIEWPORTINDEXEDFVPROC                              glViewportIndexedfv;
        extern PFNGLVIEWPORTPOSITIONWSCALENVPROC                       glViewportPositionWScaleNV;
        extern PFNGLVIEWPORTSWIZZLENVPROC                              glViewportSwizzleNV;
        extern PFNGLWAITSYNCPROC                                       glWaitSync;
        extern PFNGLWAITVKSEMAPHORENVPROC                              glWaitVkSemaphoreNV;
        extern PFNGLWEIGHTPATHSNVPROC                                  glWeightPathsNV;
        extern PFNGLWINDOWRECTANGLESEXTPROC                            glWindowRectanglesEXT;
}

inline void glActiveProgramEXT(GLuint program) noexcept
{
        opengl_functions::glActiveProgramEXT(program);
}
inline void glActiveShaderProgram(GLuint pipeline, GLuint program) noexcept
{
        opengl_functions::glActiveShaderProgram(pipeline, program);
}
inline void glActiveTexture(GLenum texture) noexcept
{
        opengl_functions::glActiveTexture(texture);
}
inline void glApplyFramebufferAttachmentCMAAINTEL() noexcept
{
        opengl_functions::glApplyFramebufferAttachmentCMAAINTEL();
}
inline void glAttachShader(GLuint program, GLuint shader) noexcept
{
        opengl_functions::glAttachShader(program, shader);
}
inline void glBeginConditionalRender(GLuint id, GLenum mode) noexcept
{
        opengl_functions::glBeginConditionalRender(id, mode);
}
inline void glBeginConditionalRenderNV(GLuint id, GLenum mode) noexcept
{
        opengl_functions::glBeginConditionalRenderNV(id, mode);
}
inline void glBeginPerfMonitorAMD(GLuint monitor) noexcept
{
        opengl_functions::glBeginPerfMonitorAMD(monitor);
}
inline void glBeginPerfQueryINTEL(GLuint queryHandle) noexcept
{
        opengl_functions::glBeginPerfQueryINTEL(queryHandle);
}
inline void glBeginQuery(GLenum target, GLuint id) noexcept
{
        opengl_functions::glBeginQuery(target, id);
}
inline void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id) noexcept
{
        opengl_functions::glBeginQueryIndexed(target, index, id);
}
inline void glBeginTransformFeedback(GLenum primitiveMode) noexcept
{
        opengl_functions::glBeginTransformFeedback(primitiveMode);
}
inline void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) noexcept
{
        opengl_functions::glBindAttribLocation(program, index, name);
}
inline void glBindBuffer(GLenum target, GLuint buffer) noexcept
{
        opengl_functions::glBindBuffer(target, buffer);
}
inline void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) noexcept
{
        opengl_functions::glBindBufferBase(target, index, buffer);
}
inline void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) noexcept
{
        opengl_functions::glBindBufferRange(target, index, buffer, offset, size);
}
inline void glBindBuffersBase(GLenum target, GLuint first, GLsizei count, const GLuint *buffers) noexcept
{
        opengl_functions::glBindBuffersBase(target, first, count, buffers);
}
inline void glBindBuffersRange(GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes) noexcept
{
        opengl_functions::glBindBuffersRange(target, first, count, buffers, offsets, sizes);
}
inline void glBindFragDataLocation(GLuint program, GLuint color, const GLchar *name) noexcept
{
        opengl_functions::glBindFragDataLocation(program, color, name);
}
inline void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name) noexcept
{
        opengl_functions::glBindFragDataLocationIndexed(program, colorNumber, index, name);
}
inline void glBindFramebuffer(GLenum target, GLuint framebuffer) noexcept
{
        opengl_functions::glBindFramebuffer(target, framebuffer);
}
inline void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) noexcept
{
        opengl_functions::glBindImageTexture(unit, texture, level, layered, layer, access, format);
}
inline void glBindImageTextures(GLuint first, GLsizei count, const GLuint *textures) noexcept
{
        opengl_functions::glBindImageTextures(first, count, textures);
}
inline void glBindMultiTextureEXT(GLenum texunit, GLenum target, GLuint texture) noexcept
{
        opengl_functions::glBindMultiTextureEXT(texunit, target, texture);
}
inline void glBindProgramPipeline(GLuint pipeline) noexcept
{
        opengl_functions::glBindProgramPipeline(pipeline);
}
inline void glBindRenderbuffer(GLenum target, GLuint renderbuffer) noexcept
{
        opengl_functions::glBindRenderbuffer(target, renderbuffer);
}
inline void glBindSampler(GLuint unit, GLuint sampler) noexcept
{
        opengl_functions::glBindSampler(unit, sampler);
}
inline void glBindSamplers(GLuint first, GLsizei count, const GLuint *samplers) noexcept
{
        opengl_functions::glBindSamplers(first, count, samplers);
}
inline void glBindShadingRateImageNV(GLuint texture) noexcept
{
        opengl_functions::glBindShadingRateImageNV(texture);
}
inline void glBindTexture(GLenum target, GLuint texture) noexcept
{
        opengl_functions::glBindTexture(target, texture);
}
inline void glBindTextureUnit(GLuint unit, GLuint texture) noexcept
{
        opengl_functions::glBindTextureUnit(unit, texture);
}
inline void glBindTextures(GLuint first, GLsizei count, const GLuint *textures) noexcept
{
        opengl_functions::glBindTextures(first, count, textures);
}
inline void glBindTransformFeedback(GLenum target, GLuint id) noexcept
{
        opengl_functions::glBindTransformFeedback(target, id);
}
inline void glBindVertexArray(GLuint array) noexcept
{
        opengl_functions::glBindVertexArray(array);
}
inline void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) noexcept
{
        opengl_functions::glBindVertexBuffer(bindingindex, buffer, offset, stride);
}
inline void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides) noexcept
{
        opengl_functions::glBindVertexBuffers(first, count, buffers, offsets, strides);
}
inline void glBlendBarrierKHR() noexcept
{
        opengl_functions::glBlendBarrierKHR();
}
inline void glBlendBarrierNV() noexcept
{
        opengl_functions::glBlendBarrierNV();
}
inline void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) noexcept
{
        opengl_functions::glBlendColor(red, green, blue, alpha);
}
inline void glBlendEquation(GLenum mode) noexcept
{
        opengl_functions::glBlendEquation(mode);
}
inline void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) noexcept
{
        opengl_functions::glBlendEquationSeparate(modeRGB, modeAlpha);
}
inline void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) noexcept
{
        opengl_functions::glBlendEquationSeparatei(buf, modeRGB, modeAlpha);
}
inline void glBlendEquationSeparateiARB(GLuint buf, GLenum modeRGB, GLenum modeAlpha) noexcept
{
        opengl_functions::glBlendEquationSeparateiARB(buf, modeRGB, modeAlpha);
}
inline void glBlendEquationi(GLuint buf, GLenum mode) noexcept
{
        opengl_functions::glBlendEquationi(buf, mode);
}
inline void glBlendEquationiARB(GLuint buf, GLenum mode) noexcept
{
        opengl_functions::glBlendEquationiARB(buf, mode);
}
inline void glBlendFunc(GLenum sfactor, GLenum dfactor) noexcept
{
        opengl_functions::glBlendFunc(sfactor, dfactor);
}
inline void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) noexcept
{
        opengl_functions::glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}
inline void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) noexcept
{
        opengl_functions::glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}
inline void glBlendFuncSeparateiARB(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) noexcept
{
        opengl_functions::glBlendFuncSeparateiARB(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}
inline void glBlendFunci(GLuint buf, GLenum src, GLenum dst) noexcept
{
        opengl_functions::glBlendFunci(buf, src, dst);
}
inline void glBlendFunciARB(GLuint buf, GLenum src, GLenum dst) noexcept
{
        opengl_functions::glBlendFunciARB(buf, src, dst);
}
inline void glBlendParameteriNV(GLenum pname, GLint value) noexcept
{
        opengl_functions::glBlendParameteriNV(pname, value);
}
inline void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) noexcept
{
        opengl_functions::glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
inline void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) noexcept
{
        opengl_functions::glBlitNamedFramebuffer(readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
inline void glBufferAddressRangeNV(GLenum pname, GLuint index, GLuint64EXT address, GLsizeiptr length) noexcept
{
        opengl_functions::glBufferAddressRangeNV(pname, index, address, length);
}
inline void glBufferAttachMemoryNV(GLenum target, GLuint memory, GLuint64 offset) noexcept
{
        opengl_functions::glBufferAttachMemoryNV(target, memory, offset);
}
inline void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) noexcept
{
        opengl_functions::glBufferData(target, size, data, usage);
}
inline void glBufferPageCommitmentARB(GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit) noexcept
{
        opengl_functions::glBufferPageCommitmentARB(target, offset, size, commit);
}
inline void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) noexcept
{
        opengl_functions::glBufferStorage(target, size, data, flags);
}
inline void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) noexcept
{
        opengl_functions::glBufferSubData(target, offset, size, data);
}
inline void glCallCommandListNV(GLuint list) noexcept
{
        opengl_functions::glCallCommandListNV(list);
}
inline GLenum glCheckFramebufferStatus(GLenum target) noexcept
{
        return opengl_functions::glCheckFramebufferStatus(target);
}
inline GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) noexcept
{
        return opengl_functions::glCheckNamedFramebufferStatus(framebuffer, target);
}
inline GLenum glCheckNamedFramebufferStatusEXT(GLuint framebuffer, GLenum target) noexcept
{
        return opengl_functions::glCheckNamedFramebufferStatusEXT(framebuffer, target);
}
inline void glClampColor(GLenum target, GLenum clamp) noexcept
{
        opengl_functions::glClampColor(target, clamp);
}
inline void glClear(GLbitfield mask) noexcept
{
        opengl_functions::glClear(mask);
}
inline void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data) noexcept
{
        opengl_functions::glClearBufferData(target, internalformat, format, type, data);
}
inline void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data) noexcept
{
        opengl_functions::glClearBufferSubData(target, internalformat, offset, size, format, type, data);
}
inline void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) noexcept
{
        opengl_functions::glClearBufferfi(buffer, drawbuffer, depth, stencil);
}
inline void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value) noexcept
{
        opengl_functions::glClearBufferfv(buffer, drawbuffer, value);
}
inline void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value) noexcept
{
        opengl_functions::glClearBufferiv(buffer, drawbuffer, value);
}
inline void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value) noexcept
{
        opengl_functions::glClearBufferuiv(buffer, drawbuffer, value);
}
inline void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) noexcept
{
        opengl_functions::glClearColor(red, green, blue, alpha);
}
inline void glClearDepth(GLdouble depth) noexcept
{
        opengl_functions::glClearDepth(depth);
}
inline void glClearDepthf(GLfloat d) noexcept
{
        opengl_functions::glClearDepthf(d);
}
inline void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data) noexcept
{
        opengl_functions::glClearNamedBufferData(buffer, internalformat, format, type, data);
}
inline void glClearNamedBufferDataEXT(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data) noexcept
{
        opengl_functions::glClearNamedBufferDataEXT(buffer, internalformat, format, type, data);
}
inline void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data) noexcept
{
        opengl_functions::glClearNamedBufferSubData(buffer, internalformat, offset, size, format, type, data);
}
inline void glClearNamedBufferSubDataEXT(GLuint buffer, GLenum internalformat, GLsizeiptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data) noexcept
{
        opengl_functions::glClearNamedBufferSubDataEXT(buffer, internalformat, offset, size, format, type, data);
}
inline void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) noexcept
{
        opengl_functions::glClearNamedFramebufferfi(framebuffer, buffer, drawbuffer, depth, stencil);
}
inline void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value) noexcept
{
        opengl_functions::glClearNamedFramebufferfv(framebuffer, buffer, drawbuffer, value);
}
inline void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value) noexcept
{
        opengl_functions::glClearNamedFramebufferiv(framebuffer, buffer, drawbuffer, value);
}
inline void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value) noexcept
{
        opengl_functions::glClearNamedFramebufferuiv(framebuffer, buffer, drawbuffer, value);
}
inline void glClearStencil(GLint s) noexcept
{
        opengl_functions::glClearStencil(s);
}
inline void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data) noexcept
{
        opengl_functions::glClearTexImage(texture, level, format, type, data);
}
inline void glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data) noexcept
{
        opengl_functions::glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
}
inline void glClientAttribDefaultEXT(GLbitfield mask) noexcept
{
        opengl_functions::glClientAttribDefaultEXT(mask);
}
inline GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) noexcept
{
        return opengl_functions::glClientWaitSync(sync, flags, timeout);
}
inline void glClipControl(GLenum origin, GLenum depth) noexcept
{
        opengl_functions::glClipControl(origin, depth);
}
inline void glColorFormatNV(GLint size, GLenum type, GLsizei stride) noexcept
{
        opengl_functions::glColorFormatNV(size, type, stride);
}
inline void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) noexcept
{
        opengl_functions::glColorMask(red, green, blue, alpha);
}
inline void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) noexcept
{
        opengl_functions::glColorMaski(index, r, g, b, a);
}
inline void glCommandListSegmentsNV(GLuint list, GLuint segments) noexcept
{
        opengl_functions::glCommandListSegmentsNV(list, segments);
}
inline void glCompileCommandListNV(GLuint list) noexcept
{
        opengl_functions::glCompileCommandListNV(list);
}
inline void glCompileShader(GLuint shader) noexcept
{
        opengl_functions::glCompileShader(shader);
}
inline void glCompileShaderIncludeARB(GLuint shader, GLsizei count, const GLchar *const*path, const GLint *length) noexcept
{
        opengl_functions::glCompileShaderIncludeARB(shader, count, path, length);
}
inline void glCompressedMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, imageSize, bits);
}
inline void glCompressedMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, imageSize, bits);
}
inline void glCompressedMultiTexImage3DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, imageSize, bits);
}
inline void glCompressedMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, imageSize, bits);
}
inline void glCompressedMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits);
}
inline void glCompressedMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits);
}
inline void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data) noexcept
{
        opengl_functions::glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);
}
inline void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) noexcept
{
        opengl_functions::glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}
inline void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data) noexcept
{
        opengl_functions::glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
}
inline void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) noexcept
{
        opengl_functions::glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);
}
inline void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) noexcept
{
        opengl_functions::glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}
inline void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) noexcept
{
        opengl_functions::glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
inline void glCompressedTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedTextureImage1DEXT(texture, target, level, internalformat, width, border, imageSize, bits);
}
inline void glCompressedTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedTextureImage2DEXT(texture, target, level, internalformat, width, height, border, imageSize, bits);
}
inline void glCompressedTextureImage3DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedTextureImage3DEXT(texture, target, level, internalformat, width, height, depth, border, imageSize, bits);
}
inline void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) noexcept
{
        opengl_functions::glCompressedTextureSubImage1D(texture, level, xoffset, width, format, imageSize, data);
}
inline void glCompressedTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedTextureSubImage1DEXT(texture, target, level, xoffset, width, format, imageSize, bits);
}
inline void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) noexcept
{
        opengl_functions::glCompressedTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, imageSize, data);
}
inline void glCompressedTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits);
}
inline void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) noexcept
{
        opengl_functions::glCompressedTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
inline void glCompressedTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits) noexcept
{
        opengl_functions::glCompressedTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits);
}
inline void glConservativeRasterParameterfNV(GLenum pname, GLfloat value) noexcept
{
        opengl_functions::glConservativeRasterParameterfNV(pname, value);
}
inline void glConservativeRasterParameteriNV(GLenum pname, GLint param) noexcept
{
        opengl_functions::glConservativeRasterParameteriNV(pname, param);
}
inline void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) noexcept
{
        opengl_functions::glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
}
inline void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) noexcept
{
        opengl_functions::glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
}
inline void glCopyMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) noexcept
{
        opengl_functions::glCopyMultiTexImage1DEXT(texunit, target, level, internalformat, x, y, width, border);
}
inline void glCopyMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) noexcept
{
        opengl_functions::glCopyMultiTexImage2DEXT(texunit, target, level, internalformat, x, y, width, height, border);
}
inline void glCopyMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) noexcept
{
        opengl_functions::glCopyMultiTexSubImage1DEXT(texunit, target, level, xoffset, x, y, width);
}
inline void glCopyMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glCopyMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, x, y, width, height);
}
inline void glCopyMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glCopyMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height);
}
inline void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) noexcept
{
        opengl_functions::glCopyNamedBufferSubData(readBuffer, writeBuffer, readOffset, writeOffset, size);
}
inline void glCopyPathNV(GLuint resultPath, GLuint srcPath) noexcept
{
        opengl_functions::glCopyPathNV(resultPath, srcPath);
}
inline void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) noexcept
{
        opengl_functions::glCopyTexImage1D(target, level, internalformat, x, y, width, border);
}
inline void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) noexcept
{
        opengl_functions::glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}
inline void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) noexcept
{
        opengl_functions::glCopyTexSubImage1D(target, level, xoffset, x, y, width);
}
inline void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}
inline void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}
inline void glCopyTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) noexcept
{
        opengl_functions::glCopyTextureImage1DEXT(texture, target, level, internalformat, x, y, width, border);
}
inline void glCopyTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) noexcept
{
        opengl_functions::glCopyTextureImage2DEXT(texture, target, level, internalformat, x, y, width, height, border);
}
inline void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) noexcept
{
        opengl_functions::glCopyTextureSubImage1D(texture, level, xoffset, x, y, width);
}
inline void glCopyTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) noexcept
{
        opengl_functions::glCopyTextureSubImage1DEXT(texture, target, level, xoffset, x, y, width);
}
inline void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glCopyTextureSubImage2D(texture, level, xoffset, yoffset, x, y, width, height);
}
inline void glCopyTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glCopyTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, x, y, width, height);
}
inline void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glCopyTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, x, y, width, height);
}
inline void glCopyTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glCopyTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, x, y, width, height);
}
inline void glCoverFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues) noexcept
{
        opengl_functions::glCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues);
}
inline void glCoverFillPathNV(GLuint path, GLenum coverMode) noexcept
{
        opengl_functions::glCoverFillPathNV(path, coverMode);
}
inline void glCoverStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues) noexcept
{
        opengl_functions::glCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues);
}
inline void glCoverStrokePathNV(GLuint path, GLenum coverMode) noexcept
{
        opengl_functions::glCoverStrokePathNV(path, coverMode);
}
inline void glCoverageModulationNV(GLenum components) noexcept
{
        opengl_functions::glCoverageModulationNV(components);
}
inline void glCoverageModulationTableNV(GLsizei n, const GLfloat *v) noexcept
{
        opengl_functions::glCoverageModulationTableNV(n, v);
}
inline void glCreateBuffers(GLsizei n, GLuint *buffers) noexcept
{
        opengl_functions::glCreateBuffers(n, buffers);
}
inline void glCreateCommandListsNV(GLsizei n, GLuint *lists) noexcept
{
        opengl_functions::glCreateCommandListsNV(n, lists);
}
inline void glCreateFramebuffers(GLsizei n, GLuint *framebuffers) noexcept
{
        opengl_functions::glCreateFramebuffers(n, framebuffers);
}
inline void glCreatePerfQueryINTEL(GLuint queryId, GLuint *queryHandle) noexcept
{
        opengl_functions::glCreatePerfQueryINTEL(queryId, queryHandle);
}
inline GLuint glCreateProgram() noexcept
{
        return opengl_functions::glCreateProgram();
}
inline void glCreateProgramPipelines(GLsizei n, GLuint *pipelines) noexcept
{
        opengl_functions::glCreateProgramPipelines(n, pipelines);
}
inline void glCreateQueries(GLenum target, GLsizei n, GLuint *ids) noexcept
{
        opengl_functions::glCreateQueries(target, n, ids);
}
inline void glCreateRenderbuffers(GLsizei n, GLuint *renderbuffers) noexcept
{
        opengl_functions::glCreateRenderbuffers(n, renderbuffers);
}
inline void glCreateSamplers(GLsizei n, GLuint *samplers) noexcept
{
        opengl_functions::glCreateSamplers(n, samplers);
}
inline GLuint glCreateShader(GLenum type) noexcept
{
        return opengl_functions::glCreateShader(type);
}
inline GLuint glCreateShaderProgramEXT(GLenum type, const GLchar *string) noexcept
{
        return opengl_functions::glCreateShaderProgramEXT(type, string);
}
inline GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const*strings) noexcept
{
        return opengl_functions::glCreateShaderProgramv(type, count, strings);
}
inline void glCreateStatesNV(GLsizei n, GLuint *states) noexcept
{
        opengl_functions::glCreateStatesNV(n, states);
}
inline GLsync glCreateSyncFromCLeventARB(struct _cl_context *context, struct _cl_event *event, GLbitfield flags) noexcept
{
        return opengl_functions::glCreateSyncFromCLeventARB(context, event, flags);
}
inline void glCreateTextures(GLenum target, GLsizei n, GLuint *textures) noexcept
{
        opengl_functions::glCreateTextures(target, n, textures);
}
inline void glCreateTransformFeedbacks(GLsizei n, GLuint *ids) noexcept
{
        opengl_functions::glCreateTransformFeedbacks(n, ids);
}
inline void glCreateVertexArrays(GLsizei n, GLuint *arrays) noexcept
{
        opengl_functions::glCreateVertexArrays(n, arrays);
}
inline void glCullFace(GLenum mode) noexcept
{
        opengl_functions::glCullFace(mode);
}
inline void glDebugMessageCallback(GLDEBUGPROC callback, const void *userParam) noexcept
{
        opengl_functions::glDebugMessageCallback(callback, userParam);
}
inline void glDebugMessageCallbackARB(GLDEBUGPROCARB callback, const void *userParam) noexcept
{
        opengl_functions::glDebugMessageCallbackARB(callback, userParam);
}
inline void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled) noexcept
{
        opengl_functions::glDebugMessageControl(source, type, severity, count, ids, enabled);
}
inline void glDebugMessageControlARB(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled) noexcept
{
        opengl_functions::glDebugMessageControlARB(source, type, severity, count, ids, enabled);
}
inline void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf) noexcept
{
        opengl_functions::glDebugMessageInsert(source, type, id, severity, length, buf);
}
inline void glDebugMessageInsertARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf) noexcept
{
        opengl_functions::glDebugMessageInsertARB(source, type, id, severity, length, buf);
}
inline void glDeleteBuffers(GLsizei n, const GLuint *buffers) noexcept
{
        opengl_functions::glDeleteBuffers(n, buffers);
}
inline void glDeleteCommandListsNV(GLsizei n, const GLuint *lists) noexcept
{
        opengl_functions::glDeleteCommandListsNV(n, lists);
}
inline void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) noexcept
{
        opengl_functions::glDeleteFramebuffers(n, framebuffers);
}
inline void glDeleteNamedStringARB(GLint namelen, const GLchar *name) noexcept
{
        opengl_functions::glDeleteNamedStringARB(namelen, name);
}
inline void glDeletePathsNV(GLuint path, GLsizei range) noexcept
{
        opengl_functions::glDeletePathsNV(path, range);
}
inline void glDeletePerfMonitorsAMD(GLsizei n, GLuint *monitors) noexcept
{
        opengl_functions::glDeletePerfMonitorsAMD(n, monitors);
}
inline void glDeletePerfQueryINTEL(GLuint queryHandle) noexcept
{
        opengl_functions::glDeletePerfQueryINTEL(queryHandle);
}
inline void glDeleteProgram(GLuint program) noexcept
{
        opengl_functions::glDeleteProgram(program);
}
inline void glDeleteProgramPipelines(GLsizei n, const GLuint *pipelines) noexcept
{
        opengl_functions::glDeleteProgramPipelines(n, pipelines);
}
inline void glDeleteQueries(GLsizei n, const GLuint *ids) noexcept
{
        opengl_functions::glDeleteQueries(n, ids);
}
inline void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) noexcept
{
        opengl_functions::glDeleteRenderbuffers(n, renderbuffers);
}
inline void glDeleteSamplers(GLsizei count, const GLuint *samplers) noexcept
{
        opengl_functions::glDeleteSamplers(count, samplers);
}
inline void glDeleteShader(GLuint shader) noexcept
{
        opengl_functions::glDeleteShader(shader);
}
inline void glDeleteStatesNV(GLsizei n, const GLuint *states) noexcept
{
        opengl_functions::glDeleteStatesNV(n, states);
}
inline void glDeleteSync(GLsync sync) noexcept
{
        opengl_functions::glDeleteSync(sync);
}
inline void glDeleteTextures(GLsizei n, const GLuint *textures) noexcept
{
        opengl_functions::glDeleteTextures(n, textures);
}
inline void glDeleteTransformFeedbacks(GLsizei n, const GLuint *ids) noexcept
{
        opengl_functions::glDeleteTransformFeedbacks(n, ids);
}
inline void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) noexcept
{
        opengl_functions::glDeleteVertexArrays(n, arrays);
}
inline void glDepthFunc(GLenum func) noexcept
{
        opengl_functions::glDepthFunc(func);
}
inline void glDepthMask(GLboolean flag) noexcept
{
        opengl_functions::glDepthMask(flag);
}
inline void glDepthRange(GLdouble n, GLdouble f) noexcept
{
        opengl_functions::glDepthRange(n, f);
}
inline void glDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble *v) noexcept
{
        opengl_functions::glDepthRangeArrayv(first, count, v);
}
inline void glDepthRangeIndexed(GLuint index, GLdouble n, GLdouble f) noexcept
{
        opengl_functions::glDepthRangeIndexed(index, n, f);
}
inline void glDepthRangef(GLfloat n, GLfloat f) noexcept
{
        opengl_functions::glDepthRangef(n, f);
}
inline void glDetachShader(GLuint program, GLuint shader) noexcept
{
        opengl_functions::glDetachShader(program, shader);
}
inline void glDisable(GLenum cap) noexcept
{
        opengl_functions::glDisable(cap);
}
inline void glDisableClientStateIndexedEXT(GLenum array, GLuint index) noexcept
{
        opengl_functions::glDisableClientStateIndexedEXT(array, index);
}
inline void glDisableClientStateiEXT(GLenum array, GLuint index) noexcept
{
        opengl_functions::glDisableClientStateiEXT(array, index);
}
inline void glDisableIndexedEXT(GLenum target, GLuint index) noexcept
{
        opengl_functions::glDisableIndexedEXT(target, index);
}
inline void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index) noexcept
{
        opengl_functions::glDisableVertexArrayAttrib(vaobj, index);
}
inline void glDisableVertexArrayAttribEXT(GLuint vaobj, GLuint index) noexcept
{
        opengl_functions::glDisableVertexArrayAttribEXT(vaobj, index);
}
inline void glDisableVertexArrayEXT(GLuint vaobj, GLenum array) noexcept
{
        opengl_functions::glDisableVertexArrayEXT(vaobj, array);
}
inline void glDisableVertexAttribArray(GLuint index) noexcept
{
        opengl_functions::glDisableVertexAttribArray(index);
}
inline void glDisablei(GLenum target, GLuint index) noexcept
{
        opengl_functions::glDisablei(target, index);
}
inline void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) noexcept
{
        opengl_functions::glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}
inline void glDispatchComputeGroupSizeARB(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z, GLuint group_size_x, GLuint group_size_y, GLuint group_size_z) noexcept
{
        opengl_functions::glDispatchComputeGroupSizeARB(num_groups_x, num_groups_y, num_groups_z, group_size_x, group_size_y, group_size_z);
}
inline void glDispatchComputeIndirect(GLintptr indirect) noexcept
{
        opengl_functions::glDispatchComputeIndirect(indirect);
}
inline void glDrawArrays(GLenum mode, GLint first, GLsizei count) noexcept
{
        opengl_functions::glDrawArrays(mode, first, count);
}
inline void glDrawArraysIndirect(GLenum mode, const void *indirect) noexcept
{
        opengl_functions::glDrawArraysIndirect(mode, indirect);
}
inline void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) noexcept
{
        opengl_functions::glDrawArraysInstanced(mode, first, count, instancecount);
}
inline void glDrawArraysInstancedARB(GLenum mode, GLint first, GLsizei count, GLsizei primcount) noexcept
{
        opengl_functions::glDrawArraysInstancedARB(mode, first, count, primcount);
}
inline void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) noexcept
{
        opengl_functions::glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance);
}
inline void glDrawArraysInstancedEXT(GLenum mode, GLint start, GLsizei count, GLsizei primcount) noexcept
{
        opengl_functions::glDrawArraysInstancedEXT(mode, start, count, primcount);
}
inline void glDrawBuffer(GLenum buf) noexcept
{
        opengl_functions::glDrawBuffer(buf);
}
inline void glDrawBuffers(GLsizei n, const GLenum *bufs) noexcept
{
        opengl_functions::glDrawBuffers(n, bufs);
}
inline void glDrawCommandsAddressNV(GLenum primitiveMode, const GLuint64 *indirects, const GLsizei *sizes, GLuint count) noexcept
{
        opengl_functions::glDrawCommandsAddressNV(primitiveMode, indirects, sizes, count);
}
inline void glDrawCommandsNV(GLenum primitiveMode, GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, GLuint count) noexcept
{
        opengl_functions::glDrawCommandsNV(primitiveMode, buffer, indirects, sizes, count);
}
inline void glDrawCommandsStatesAddressNV(const GLuint64 *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count) noexcept
{
        opengl_functions::glDrawCommandsStatesAddressNV(indirects, sizes, states, fbos, count);
}
inline void glDrawCommandsStatesNV(GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count) noexcept
{
        opengl_functions::glDrawCommandsStatesNV(buffer, indirects, sizes, states, fbos, count);
}
inline void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) noexcept
{
        opengl_functions::glDrawElements(mode, count, type, indices);
}
inline void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) noexcept
{
        opengl_functions::glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
}
inline void glDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect) noexcept
{
        opengl_functions::glDrawElementsIndirect(mode, type, indirect);
}
inline void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) noexcept
{
        opengl_functions::glDrawElementsInstanced(mode, count, type, indices, instancecount);
}
inline void glDrawElementsInstancedARB(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount) noexcept
{
        opengl_functions::glDrawElementsInstancedARB(mode, count, type, indices, primcount);
}
inline void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance) noexcept
{
        opengl_functions::glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance);
}
inline void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) noexcept
{
        opengl_functions::glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex);
}
inline void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) noexcept
{
        opengl_functions::glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance);
}
inline void glDrawElementsInstancedEXT(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount) noexcept
{
        opengl_functions::glDrawElementsInstancedEXT(mode, count, type, indices, primcount);
}
inline void glDrawMeshTasksIndirectNV(GLintptr indirect) noexcept
{
        opengl_functions::glDrawMeshTasksIndirectNV(indirect);
}
inline void glDrawMeshTasksNV(GLuint first, GLuint count) noexcept
{
        opengl_functions::glDrawMeshTasksNV(first, count);
}
inline void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) noexcept
{
        opengl_functions::glDrawRangeElements(mode, start, end, count, type, indices);
}
inline void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex) noexcept
{
        opengl_functions::glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
}
inline void glDrawTransformFeedback(GLenum mode, GLuint id) noexcept
{
        opengl_functions::glDrawTransformFeedback(mode, id);
}
inline void glDrawTransformFeedbackInstanced(GLenum mode, GLuint id, GLsizei instancecount) noexcept
{
        opengl_functions::glDrawTransformFeedbackInstanced(mode, id, instancecount);
}
inline void glDrawTransformFeedbackStream(GLenum mode, GLuint id, GLuint stream) noexcept
{
        opengl_functions::glDrawTransformFeedbackStream(mode, id, stream);
}
inline void glDrawTransformFeedbackStreamInstanced(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount) noexcept
{
        opengl_functions::glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount);
}
inline void glDrawVkImageNV(GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1) noexcept
{
        opengl_functions::glDrawVkImageNV(vkImage, sampler, x0, y0, x1, y1, z, s0, t0, s1, t1);
}
inline void glEGLImageTargetTexStorageEXT(GLenum target, GLeglImageOES image, const GLint* attrib_list) noexcept
{
        opengl_functions::glEGLImageTargetTexStorageEXT(target, image, attrib_list);
}
inline void glEGLImageTargetTextureStorageEXT(GLuint texture, GLeglImageOES image, const GLint* attrib_list) noexcept
{
        opengl_functions::glEGLImageTargetTextureStorageEXT(texture, image, attrib_list);
}
inline void glEdgeFlagFormatNV(GLsizei stride) noexcept
{
        opengl_functions::glEdgeFlagFormatNV(stride);
}
inline void glEnable(GLenum cap) noexcept
{
        opengl_functions::glEnable(cap);
}
inline void glEnableClientStateIndexedEXT(GLenum array, GLuint index) noexcept
{
        opengl_functions::glEnableClientStateIndexedEXT(array, index);
}
inline void glEnableClientStateiEXT(GLenum array, GLuint index) noexcept
{
        opengl_functions::glEnableClientStateiEXT(array, index);
}
inline void glEnableIndexedEXT(GLenum target, GLuint index) noexcept
{
        opengl_functions::glEnableIndexedEXT(target, index);
}
inline void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index) noexcept
{
        opengl_functions::glEnableVertexArrayAttrib(vaobj, index);
}
inline void glEnableVertexArrayAttribEXT(GLuint vaobj, GLuint index) noexcept
{
        opengl_functions::glEnableVertexArrayAttribEXT(vaobj, index);
}
inline void glEnableVertexArrayEXT(GLuint vaobj, GLenum array) noexcept
{
        opengl_functions::glEnableVertexArrayEXT(vaobj, array);
}
inline void glEnableVertexAttribArray(GLuint index) noexcept
{
        opengl_functions::glEnableVertexAttribArray(index);
}
inline void glEnablei(GLenum target, GLuint index) noexcept
{
        opengl_functions::glEnablei(target, index);
}
inline void glEndConditionalRender() noexcept
{
        opengl_functions::glEndConditionalRender();
}
inline void glEndConditionalRenderNV() noexcept
{
        opengl_functions::glEndConditionalRenderNV();
}
inline void glEndPerfMonitorAMD(GLuint monitor) noexcept
{
        opengl_functions::glEndPerfMonitorAMD(monitor);
}
inline void glEndPerfQueryINTEL(GLuint queryHandle) noexcept
{
        opengl_functions::glEndPerfQueryINTEL(queryHandle);
}
inline void glEndQuery(GLenum target) noexcept
{
        opengl_functions::glEndQuery(target);
}
inline void glEndQueryIndexed(GLenum target, GLuint index) noexcept
{
        opengl_functions::glEndQueryIndexed(target, index);
}
inline void glEndTransformFeedback() noexcept
{
        opengl_functions::glEndTransformFeedback();
}
inline void glEvaluateDepthValuesARB() noexcept
{
        opengl_functions::glEvaluateDepthValuesARB();
}
inline GLsync glFenceSync(GLenum condition, GLbitfield flags) noexcept
{
        return opengl_functions::glFenceSync(condition, flags);
}
inline void glFinish() noexcept
{
        opengl_functions::glFinish();
}
inline void glFlush() noexcept
{
        opengl_functions::glFlush();
}
inline void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) noexcept
{
        opengl_functions::glFlushMappedBufferRange(target, offset, length);
}
inline void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length) noexcept
{
        opengl_functions::glFlushMappedNamedBufferRange(buffer, offset, length);
}
inline void glFlushMappedNamedBufferRangeEXT(GLuint buffer, GLintptr offset, GLsizeiptr length) noexcept
{
        opengl_functions::glFlushMappedNamedBufferRangeEXT(buffer, offset, length);
}
inline void glFogCoordFormatNV(GLenum type, GLsizei stride) noexcept
{
        opengl_functions::glFogCoordFormatNV(type, stride);
}
inline void glFragmentCoverageColorNV(GLuint color) noexcept
{
        opengl_functions::glFragmentCoverageColorNV(color);
}
inline void glFramebufferDrawBufferEXT(GLuint framebuffer, GLenum mode) noexcept
{
        opengl_functions::glFramebufferDrawBufferEXT(framebuffer, mode);
}
inline void glFramebufferDrawBuffersEXT(GLuint framebuffer, GLsizei n, const GLenum *bufs) noexcept
{
        opengl_functions::glFramebufferDrawBuffersEXT(framebuffer, n, bufs);
}
inline void glFramebufferFetchBarrierEXT() noexcept
{
        opengl_functions::glFramebufferFetchBarrierEXT();
}
inline void glFramebufferParameteri(GLenum target, GLenum pname, GLint param) noexcept
{
        opengl_functions::glFramebufferParameteri(target, pname, param);
}
inline void glFramebufferReadBufferEXT(GLuint framebuffer, GLenum mode) noexcept
{
        opengl_functions::glFramebufferReadBufferEXT(framebuffer, mode);
}
inline void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) noexcept
{
        opengl_functions::glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}
inline void glFramebufferSampleLocationsfvARB(GLenum target, GLuint start, GLsizei count, const GLfloat *v) noexcept
{
        opengl_functions::glFramebufferSampleLocationsfvARB(target, start, count, v);
}
inline void glFramebufferSampleLocationsfvNV(GLenum target, GLuint start, GLsizei count, const GLfloat *v) noexcept
{
        opengl_functions::glFramebufferSampleLocationsfvNV(target, start, count, v);
}
inline void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) noexcept
{
        opengl_functions::glFramebufferTexture(target, attachment, texture, level);
}
inline void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) noexcept
{
        opengl_functions::glFramebufferTexture1D(target, attachment, textarget, texture, level);
}
inline void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) noexcept
{
        opengl_functions::glFramebufferTexture2D(target, attachment, textarget, texture, level);
}
inline void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) noexcept
{
        opengl_functions::glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
}
inline void glFramebufferTextureARB(GLenum target, GLenum attachment, GLuint texture, GLint level) noexcept
{
        opengl_functions::glFramebufferTextureARB(target, attachment, texture, level);
}
inline void glFramebufferTextureFaceARB(GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face) noexcept
{
        opengl_functions::glFramebufferTextureFaceARB(target, attachment, texture, level, face);
}
inline void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) noexcept
{
        opengl_functions::glFramebufferTextureLayer(target, attachment, texture, level, layer);
}
inline void glFramebufferTextureLayerARB(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) noexcept
{
        opengl_functions::glFramebufferTextureLayerARB(target, attachment, texture, level, layer);
}
inline void glFramebufferTextureMultiviewOVR(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews) noexcept
{
        opengl_functions::glFramebufferTextureMultiviewOVR(target, attachment, texture, level, baseViewIndex, numViews);
}
inline void glFrontFace(GLenum mode) noexcept
{
        opengl_functions::glFrontFace(mode);
}
inline void glGenBuffers(GLsizei n, GLuint *buffers) noexcept
{
        opengl_functions::glGenBuffers(n, buffers);
}
inline void glGenFramebuffers(GLsizei n, GLuint *framebuffers) noexcept
{
        opengl_functions::glGenFramebuffers(n, framebuffers);
}
inline GLuint glGenPathsNV(GLsizei range) noexcept
{
        return opengl_functions::glGenPathsNV(range);
}
inline void glGenPerfMonitorsAMD(GLsizei n, GLuint *monitors) noexcept
{
        opengl_functions::glGenPerfMonitorsAMD(n, monitors);
}
inline void glGenProgramPipelines(GLsizei n, GLuint *pipelines) noexcept
{
        opengl_functions::glGenProgramPipelines(n, pipelines);
}
inline void glGenQueries(GLsizei n, GLuint *ids) noexcept
{
        opengl_functions::glGenQueries(n, ids);
}
inline void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) noexcept
{
        opengl_functions::glGenRenderbuffers(n, renderbuffers);
}
inline void glGenSamplers(GLsizei count, GLuint *samplers) noexcept
{
        opengl_functions::glGenSamplers(count, samplers);
}
inline void glGenTextures(GLsizei n, GLuint *textures) noexcept
{
        opengl_functions::glGenTextures(n, textures);
}
inline void glGenTransformFeedbacks(GLsizei n, GLuint *ids) noexcept
{
        opengl_functions::glGenTransformFeedbacks(n, ids);
}
inline void glGenVertexArrays(GLsizei n, GLuint *arrays) noexcept
{
        opengl_functions::glGenVertexArrays(n, arrays);
}
inline void glGenerateMipmap(GLenum target) noexcept
{
        opengl_functions::glGenerateMipmap(target);
}
inline void glGenerateMultiTexMipmapEXT(GLenum texunit, GLenum target) noexcept
{
        opengl_functions::glGenerateMultiTexMipmapEXT(texunit, target);
}
inline void glGenerateTextureMipmap(GLuint texture) noexcept
{
        opengl_functions::glGenerateTextureMipmap(texture);
}
inline void glGenerateTextureMipmapEXT(GLuint texture, GLenum target) noexcept
{
        opengl_functions::glGenerateTextureMipmapEXT(texture, target);
}
inline void glGetActiveAtomicCounterBufferiv(GLuint program, GLuint bufferIndex, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetActiveAtomicCounterBufferiv(program, bufferIndex, pname, params);
}
inline void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) noexcept
{
        opengl_functions::glGetActiveAttrib(program, index, bufSize, length, size, type, name);
}
inline void glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name) noexcept
{
        opengl_functions::glGetActiveSubroutineName(program, shadertype, index, bufsize, length, name);
}
inline void glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name) noexcept
{
        opengl_functions::glGetActiveSubroutineUniformName(program, shadertype, index, bufsize, length, name);
}
inline void glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values) noexcept
{
        opengl_functions::glGetActiveSubroutineUniformiv(program, shadertype, index, pname, values);
}
inline void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) noexcept
{
        opengl_functions::glGetActiveUniform(program, index, bufSize, length, size, type, name);
}
inline void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) noexcept
{
        opengl_functions::glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
}
inline void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
}
inline void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName) noexcept
{
        opengl_functions::glGetActiveUniformName(program, uniformIndex, bufSize, length, uniformName);
}
inline void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
}
inline void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) noexcept
{
        opengl_functions::glGetAttachedShaders(program, maxCount, count, shaders);
}
inline GLint glGetAttribLocation(GLuint program, const GLchar *name) noexcept
{
        return opengl_functions::glGetAttribLocation(program, name);
}
inline void glGetBooleanIndexedvEXT(GLenum target, GLuint index, GLboolean *data) noexcept
{
        opengl_functions::glGetBooleanIndexedvEXT(target, index, data);
}
inline void glGetBooleani_v(GLenum target, GLuint index, GLboolean *data) noexcept
{
        opengl_functions::glGetBooleani_v(target, index, data);
}
inline void glGetBooleanv(GLenum pname, GLboolean *data) noexcept
{
        opengl_functions::glGetBooleanv(pname, data);
}
inline void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params) noexcept
{
        opengl_functions::glGetBufferParameteri64v(target, pname, params);
}
inline void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetBufferParameteriv(target, pname, params);
}
inline void glGetBufferParameterui64vNV(GLenum target, GLenum pname, GLuint64EXT *params) noexcept
{
        opengl_functions::glGetBufferParameterui64vNV(target, pname, params);
}
inline void glGetBufferPointerv(GLenum target, GLenum pname, void **params) noexcept
{
        opengl_functions::glGetBufferPointerv(target, pname, params);
}
inline void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data) noexcept
{
        opengl_functions::glGetBufferSubData(target, offset, size, data);
}
inline GLuint glGetCommandHeaderNV(GLenum tokenID, GLuint size) noexcept
{
        return opengl_functions::glGetCommandHeaderNV(tokenID, size);
}
inline void glGetCompressedMultiTexImageEXT(GLenum texunit, GLenum target, GLint lod, void *img) noexcept
{
        opengl_functions::glGetCompressedMultiTexImageEXT(texunit, target, lod, img);
}
inline void glGetCompressedTexImage(GLenum target, GLint level, void *img) noexcept
{
        opengl_functions::glGetCompressedTexImage(target, level, img);
}
inline void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void *pixels) noexcept
{
        opengl_functions::glGetCompressedTextureImage(texture, level, bufSize, pixels);
}
inline void glGetCompressedTextureImageEXT(GLuint texture, GLenum target, GLint lod, void *img) noexcept
{
        opengl_functions::glGetCompressedTextureImageEXT(texture, target, lod, img);
}
inline void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels) noexcept
{
        opengl_functions::glGetCompressedTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, pixels);
}
inline void glGetCoverageModulationTableNV(GLsizei bufsize, GLfloat *v) noexcept
{
        opengl_functions::glGetCoverageModulationTableNV(bufsize, v);
}
inline GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog) noexcept
{
        return opengl_functions::glGetDebugMessageLog(count, bufSize, sources, types, ids, severities, lengths, messageLog);
}
inline GLuint glGetDebugMessageLogARB(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog) noexcept
{
        return opengl_functions::glGetDebugMessageLogARB(count, bufSize, sources, types, ids, severities, lengths, messageLog);
}
inline void glGetDoubleIndexedvEXT(GLenum target, GLuint index, GLdouble *data) noexcept
{
        opengl_functions::glGetDoubleIndexedvEXT(target, index, data);
}
inline void glGetDoublei_v(GLenum target, GLuint index, GLdouble *data) noexcept
{
        opengl_functions::glGetDoublei_v(target, index, data);
}
inline void glGetDoublei_vEXT(GLenum pname, GLuint index, GLdouble *params) noexcept
{
        opengl_functions::glGetDoublei_vEXT(pname, index, params);
}
inline void glGetDoublev(GLenum pname, GLdouble *data) noexcept
{
        opengl_functions::glGetDoublev(pname, data);
}
inline GLenum glGetError() noexcept
{
        return opengl_functions::glGetError();
}
inline void glGetFirstPerfQueryIdINTEL(GLuint *queryId) noexcept
{
        opengl_functions::glGetFirstPerfQueryIdINTEL(queryId);
}
inline void glGetFloatIndexedvEXT(GLenum target, GLuint index, GLfloat *data) noexcept
{
        opengl_functions::glGetFloatIndexedvEXT(target, index, data);
}
inline void glGetFloati_v(GLenum target, GLuint index, GLfloat *data) noexcept
{
        opengl_functions::glGetFloati_v(target, index, data);
}
inline void glGetFloati_vEXT(GLenum pname, GLuint index, GLfloat *params) noexcept
{
        opengl_functions::glGetFloati_vEXT(pname, index, params);
}
inline void glGetFloatv(GLenum pname, GLfloat *data) noexcept
{
        opengl_functions::glGetFloatv(pname, data);
}
inline GLint glGetFragDataIndex(GLuint program, const GLchar *name) noexcept
{
        return opengl_functions::glGetFragDataIndex(program, name);
}
inline GLint glGetFragDataLocation(GLuint program, const GLchar *name) noexcept
{
        return opengl_functions::glGetFragDataLocation(program, name);
}
inline void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}
inline void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetFramebufferParameteriv(target, pname, params);
}
inline void glGetFramebufferParameterivEXT(GLuint framebuffer, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetFramebufferParameterivEXT(framebuffer, pname, params);
}
inline GLenum glGetGraphicsResetStatus() noexcept
{
        return opengl_functions::glGetGraphicsResetStatus();
}
inline GLenum glGetGraphicsResetStatusARB() noexcept
{
        return opengl_functions::glGetGraphicsResetStatusARB();
}
inline GLuint64 glGetImageHandleARB(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format) noexcept
{
        return opengl_functions::glGetImageHandleARB(texture, level, layered, layer, format);
}
inline GLuint64 glGetImageHandleNV(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format) noexcept
{
        return opengl_functions::glGetImageHandleNV(texture, level, layered, layer, format);
}
inline void glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data) noexcept
{
        opengl_functions::glGetInteger64i_v(target, index, data);
}
inline void glGetInteger64v(GLenum pname, GLint64 *data) noexcept
{
        opengl_functions::glGetInteger64v(pname, data);
}
inline void glGetIntegerIndexedvEXT(GLenum target, GLuint index, GLint *data) noexcept
{
        opengl_functions::glGetIntegerIndexedvEXT(target, index, data);
}
inline void glGetIntegeri_v(GLenum target, GLuint index, GLint *data) noexcept
{
        opengl_functions::glGetIntegeri_v(target, index, data);
}
inline void glGetIntegerui64i_vNV(GLenum value, GLuint index, GLuint64EXT *result) noexcept
{
        opengl_functions::glGetIntegerui64i_vNV(value, index, result);
}
inline void glGetIntegerui64vNV(GLenum value, GLuint64EXT *result) noexcept
{
        opengl_functions::glGetIntegerui64vNV(value, result);
}
inline void glGetIntegerv(GLenum pname, GLint *data) noexcept
{
        opengl_functions::glGetIntegerv(pname, data);
}
inline void glGetInternalformatSampleivNV(GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei bufSize, GLint *params) noexcept
{
        opengl_functions::glGetInternalformatSampleivNV(target, internalformat, samples, pname, bufSize, params);
}
inline void glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint64 *params) noexcept
{
        opengl_functions::glGetInternalformati64v(target, internalformat, pname, bufSize, params);
}
inline void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params) noexcept
{
        opengl_functions::glGetInternalformativ(target, internalformat, pname, bufSize, params);
}
inline void glGetMemoryObjectDetachedResourcesuivNV(GLuint memory, GLenum pname, GLint first, GLsizei count, GLuint *params) noexcept
{
        opengl_functions::glGetMemoryObjectDetachedResourcesuivNV(memory, pname, first, count, params);
}
inline void glGetMultiTexEnvfvEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetMultiTexEnvfvEXT(texunit, target, pname, params);
}
inline void glGetMultiTexEnvivEXT(GLenum texunit, GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetMultiTexEnvivEXT(texunit, target, pname, params);
}
inline void glGetMultiTexGendvEXT(GLenum texunit, GLenum coord, GLenum pname, GLdouble *params) noexcept
{
        opengl_functions::glGetMultiTexGendvEXT(texunit, coord, pname, params);
}
inline void glGetMultiTexGenfvEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetMultiTexGenfvEXT(texunit, coord, pname, params);
}
inline void glGetMultiTexGenivEXT(GLenum texunit, GLenum coord, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetMultiTexGenivEXT(texunit, coord, pname, params);
}
inline void glGetMultiTexImageEXT(GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void *pixels) noexcept
{
        opengl_functions::glGetMultiTexImageEXT(texunit, target, level, format, type, pixels);
}
inline void glGetMultiTexLevelParameterfvEXT(GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetMultiTexLevelParameterfvEXT(texunit, target, level, pname, params);
}
inline void glGetMultiTexLevelParameterivEXT(GLenum texunit, GLenum target, GLint level, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetMultiTexLevelParameterivEXT(texunit, target, level, pname, params);
}
inline void glGetMultiTexParameterIivEXT(GLenum texunit, GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetMultiTexParameterIivEXT(texunit, target, pname, params);
}
inline void glGetMultiTexParameterIuivEXT(GLenum texunit, GLenum target, GLenum pname, GLuint *params) noexcept
{
        opengl_functions::glGetMultiTexParameterIuivEXT(texunit, target, pname, params);
}
inline void glGetMultiTexParameterfvEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetMultiTexParameterfvEXT(texunit, target, pname, params);
}
inline void glGetMultiTexParameterivEXT(GLenum texunit, GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetMultiTexParameterivEXT(texunit, target, pname, params);
}
inline void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat *val) noexcept
{
        opengl_functions::glGetMultisamplefv(pname, index, val);
}
inline void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64 *params) noexcept
{
        opengl_functions::glGetNamedBufferParameteri64v(buffer, pname, params);
}
inline void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetNamedBufferParameteriv(buffer, pname, params);
}
inline void glGetNamedBufferParameterivEXT(GLuint buffer, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetNamedBufferParameterivEXT(buffer, pname, params);
}
inline void glGetNamedBufferParameterui64vNV(GLuint buffer, GLenum pname, GLuint64EXT *params) noexcept
{
        opengl_functions::glGetNamedBufferParameterui64vNV(buffer, pname, params);
}
inline void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void **params) noexcept
{
        opengl_functions::glGetNamedBufferPointerv(buffer, pname, params);
}
inline void glGetNamedBufferPointervEXT(GLuint buffer, GLenum pname, void **params) noexcept
{
        opengl_functions::glGetNamedBufferPointervEXT(buffer, pname, params);
}
inline void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data) noexcept
{
        opengl_functions::glGetNamedBufferSubData(buffer, offset, size, data);
}
inline void glGetNamedBufferSubDataEXT(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data) noexcept
{
        opengl_functions::glGetNamedBufferSubDataEXT(buffer, offset, size, data);
}
inline void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetNamedFramebufferAttachmentParameteriv(framebuffer, attachment, pname, params);
}
inline void glGetNamedFramebufferAttachmentParameterivEXT(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetNamedFramebufferAttachmentParameterivEXT(framebuffer, attachment, pname, params);
}
inline void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint *param) noexcept
{
        opengl_functions::glGetNamedFramebufferParameteriv(framebuffer, pname, param);
}
inline void glGetNamedFramebufferParameterivEXT(GLuint framebuffer, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetNamedFramebufferParameterivEXT(framebuffer, pname, params);
}
inline void glGetNamedProgramLocalParameterIivEXT(GLuint program, GLenum target, GLuint index, GLint *params) noexcept
{
        opengl_functions::glGetNamedProgramLocalParameterIivEXT(program, target, index, params);
}
inline void glGetNamedProgramLocalParameterIuivEXT(GLuint program, GLenum target, GLuint index, GLuint *params) noexcept
{
        opengl_functions::glGetNamedProgramLocalParameterIuivEXT(program, target, index, params);
}
inline void glGetNamedProgramLocalParameterdvEXT(GLuint program, GLenum target, GLuint index, GLdouble *params) noexcept
{
        opengl_functions::glGetNamedProgramLocalParameterdvEXT(program, target, index, params);
}
inline void glGetNamedProgramLocalParameterfvEXT(GLuint program, GLenum target, GLuint index, GLfloat *params) noexcept
{
        opengl_functions::glGetNamedProgramLocalParameterfvEXT(program, target, index, params);
}
inline void glGetNamedProgramStringEXT(GLuint program, GLenum target, GLenum pname, void *string) noexcept
{
        opengl_functions::glGetNamedProgramStringEXT(program, target, pname, string);
}
inline void glGetNamedProgramivEXT(GLuint program, GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetNamedProgramivEXT(program, target, pname, params);
}
inline void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetNamedRenderbufferParameteriv(renderbuffer, pname, params);
}
inline void glGetNamedRenderbufferParameterivEXT(GLuint renderbuffer, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetNamedRenderbufferParameterivEXT(renderbuffer, pname, params);
}
inline void glGetNamedStringARB(GLint namelen, const GLchar *name, GLsizei bufSize, GLint *stringlen, GLchar *string) noexcept
{
        opengl_functions::glGetNamedStringARB(namelen, name, bufSize, stringlen, string);
}
inline void glGetNamedStringivARB(GLint namelen, const GLchar *name, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetNamedStringivARB(namelen, name, pname, params);
}
inline void glGetNextPerfQueryIdINTEL(GLuint queryId, GLuint *nextQueryId) noexcept
{
        opengl_functions::glGetNextPerfQueryIdINTEL(queryId, nextQueryId);
}
inline void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label) noexcept
{
        opengl_functions::glGetObjectLabel(identifier, name, bufSize, length, label);
}
inline void glGetObjectLabelEXT(GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label) noexcept
{
        opengl_functions::glGetObjectLabelEXT(type, object, bufSize, length, label);
}
inline void glGetObjectPtrLabel(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label) noexcept
{
        opengl_functions::glGetObjectPtrLabel(ptr, bufSize, length, label);
}
inline void glGetPathCommandsNV(GLuint path, GLubyte *commands) noexcept
{
        opengl_functions::glGetPathCommandsNV(path, commands);
}
inline void glGetPathCoordsNV(GLuint path, GLfloat *coords) noexcept
{
        opengl_functions::glGetPathCoordsNV(path, coords);
}
inline void glGetPathDashArrayNV(GLuint path, GLfloat *dashArray) noexcept
{
        opengl_functions::glGetPathDashArrayNV(path, dashArray);
}
inline GLfloat glGetPathLengthNV(GLuint path, GLsizei startSegment, GLsizei numSegments) noexcept
{
        return opengl_functions::glGetPathLengthNV(path, startSegment, numSegments);
}
inline void glGetPathMetricRangeNV(GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics) noexcept
{
        opengl_functions::glGetPathMetricRangeNV(metricQueryMask, firstPathName, numPaths, stride, metrics);
}
inline void glGetPathMetricsNV(GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics) noexcept
{
        opengl_functions::glGetPathMetricsNV(metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics);
}
inline void glGetPathParameterfvNV(GLuint path, GLenum pname, GLfloat *value) noexcept
{
        opengl_functions::glGetPathParameterfvNV(path, pname, value);
}
inline void glGetPathParameterivNV(GLuint path, GLenum pname, GLint *value) noexcept
{
        opengl_functions::glGetPathParameterivNV(path, pname, value);
}
inline void glGetPathSpacingNV(GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing) noexcept
{
        opengl_functions::glGetPathSpacingNV(pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing);
}
inline void glGetPerfCounterInfoINTEL(GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue) noexcept
{
        opengl_functions::glGetPerfCounterInfoINTEL(queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, counterOffset, counterDataSize, counterTypeEnum, counterDataTypeEnum, rawCounterMaxValue);
}
inline void glGetPerfMonitorCounterDataAMD(GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten) noexcept
{
        opengl_functions::glGetPerfMonitorCounterDataAMD(monitor, pname, dataSize, data, bytesWritten);
}
inline void glGetPerfMonitorCounterInfoAMD(GLuint group, GLuint counter, GLenum pname, void *data) noexcept
{
        opengl_functions::glGetPerfMonitorCounterInfoAMD(group, counter, pname, data);
}
inline void glGetPerfMonitorCounterStringAMD(GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString) noexcept
{
        opengl_functions::glGetPerfMonitorCounterStringAMD(group, counter, bufSize, length, counterString);
}
inline void glGetPerfMonitorCountersAMD(GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters) noexcept
{
        opengl_functions::glGetPerfMonitorCountersAMD(group, numCounters, maxActiveCounters, counterSize, counters);
}
inline void glGetPerfMonitorGroupStringAMD(GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString) noexcept
{
        opengl_functions::glGetPerfMonitorGroupStringAMD(group, bufSize, length, groupString);
}
inline void glGetPerfMonitorGroupsAMD(GLint *numGroups, GLsizei groupsSize, GLuint *groups) noexcept
{
        opengl_functions::glGetPerfMonitorGroupsAMD(numGroups, groupsSize, groups);
}
inline void glGetPerfQueryDataINTEL(GLuint queryHandle, GLuint flags, GLsizei dataSize, void *data, GLuint *bytesWritten) noexcept
{
        opengl_functions::glGetPerfQueryDataINTEL(queryHandle, flags, dataSize, data, bytesWritten);
}
inline void glGetPerfQueryIdByNameINTEL(GLchar *queryName, GLuint *queryId) noexcept
{
        opengl_functions::glGetPerfQueryIdByNameINTEL(queryName, queryId);
}
inline void glGetPerfQueryInfoINTEL(GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask) noexcept
{
        opengl_functions::glGetPerfQueryInfoINTEL(queryId, queryNameLength, queryName, dataSize, noCounters, noInstances, capsMask);
}
inline void glGetPointerIndexedvEXT(GLenum target, GLuint index, void **data) noexcept
{
        opengl_functions::glGetPointerIndexedvEXT(target, index, data);
}
inline void glGetPointeri_vEXT(GLenum pname, GLuint index, void **params) noexcept
{
        opengl_functions::glGetPointeri_vEXT(pname, index, params);
}
inline void glGetPointerv(GLenum pname, void **params) noexcept
{
        opengl_functions::glGetPointerv(pname, params);
}
inline void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary) noexcept
{
        opengl_functions::glGetProgramBinary(program, bufSize, length, binaryFormat, binary);
}
inline void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) noexcept
{
        opengl_functions::glGetProgramInfoLog(program, bufSize, length, infoLog);
}
inline void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetProgramInterfaceiv(program, programInterface, pname, params);
}
inline void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog) noexcept
{
        opengl_functions::glGetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog);
}
inline void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetProgramPipelineiv(pipeline, pname, params);
}
inline GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar *name) noexcept
{
        return opengl_functions::glGetProgramResourceIndex(program, programInterface, name);
}
inline GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar *name) noexcept
{
        return opengl_functions::glGetProgramResourceLocation(program, programInterface, name);
}
inline GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar *name) noexcept
{
        return opengl_functions::glGetProgramResourceLocationIndex(program, programInterface, name);
}
inline void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) noexcept
{
        opengl_functions::glGetProgramResourceName(program, programInterface, index, bufSize, length, name);
}
inline void glGetProgramResourcefvNV(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLfloat *params) noexcept
{
        opengl_functions::glGetProgramResourcefvNV(program, programInterface, index, propCount, props, bufSize, length, params);
}
inline void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params) noexcept
{
        opengl_functions::glGetProgramResourceiv(program, programInterface, index, propCount, props, bufSize, length, params);
}
inline void glGetProgramStageiv(GLuint program, GLenum shadertype, GLenum pname, GLint *values) noexcept
{
        opengl_functions::glGetProgramStageiv(program, shadertype, pname, values);
}
inline void glGetProgramiv(GLuint program, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetProgramiv(program, pname, params);
}
inline void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) noexcept
{
        opengl_functions::glGetQueryBufferObjecti64v(id, buffer, pname, offset);
}
inline void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) noexcept
{
        opengl_functions::glGetQueryBufferObjectiv(id, buffer, pname, offset);
}
inline void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) noexcept
{
        opengl_functions::glGetQueryBufferObjectui64v(id, buffer, pname, offset);
}
inline void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) noexcept
{
        opengl_functions::glGetQueryBufferObjectuiv(id, buffer, pname, offset);
}
inline void glGetQueryIndexediv(GLenum target, GLuint index, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetQueryIndexediv(target, index, pname, params);
}
inline void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 *params) noexcept
{
        opengl_functions::glGetQueryObjecti64v(id, pname, params);
}
inline void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetQueryObjectiv(id, pname, params);
}
inline void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params) noexcept
{
        opengl_functions::glGetQueryObjectui64v(id, pname, params);
}
inline void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params) noexcept
{
        opengl_functions::glGetQueryObjectuiv(id, pname, params);
}
inline void glGetQueryiv(GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetQueryiv(target, pname, params);
}
inline void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetRenderbufferParameteriv(target, pname, params);
}
inline void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetSamplerParameterIiv(sampler, pname, params);
}
inline void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint *params) noexcept
{
        opengl_functions::glGetSamplerParameterIuiv(sampler, pname, params);
}
inline void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetSamplerParameterfv(sampler, pname, params);
}
inline void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetSamplerParameteriv(sampler, pname, params);
}
inline void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) noexcept
{
        opengl_functions::glGetShaderInfoLog(shader, bufSize, length, infoLog);
}
inline void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) noexcept
{
        opengl_functions::glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
}
inline void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) noexcept
{
        opengl_functions::glGetShaderSource(shader, bufSize, length, source);
}
inline void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetShaderiv(shader, pname, params);
}
inline void glGetShadingRateImagePaletteNV(GLuint viewport, GLuint entry, GLenum *rate) noexcept
{
        opengl_functions::glGetShadingRateImagePaletteNV(viewport, entry, rate);
}
inline void glGetShadingRateSampleLocationivNV(GLenum rate, GLuint samples, GLuint index, GLint *location) noexcept
{
        opengl_functions::glGetShadingRateSampleLocationivNV(rate, samples, index, location);
}
inline GLushort glGetStageIndexNV(GLenum shadertype) noexcept
{
        return opengl_functions::glGetStageIndexNV(shadertype);
}
inline const GLubyte * glGetString(GLenum name) noexcept
{
        return opengl_functions::glGetString(name);
}
inline const GLubyte * glGetStringi(GLenum name, GLuint index) noexcept
{
        return opengl_functions::glGetStringi(name, index);
}
inline GLuint glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar *name) noexcept
{
        return opengl_functions::glGetSubroutineIndex(program, shadertype, name);
}
inline GLint glGetSubroutineUniformLocation(GLuint program, GLenum shadertype, const GLchar *name) noexcept
{
        return opengl_functions::glGetSubroutineUniformLocation(program, shadertype, name);
}
inline void glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values) noexcept
{
        opengl_functions::glGetSynciv(sync, pname, bufSize, length, values);
}
inline void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *pixels) noexcept
{
        opengl_functions::glGetTexImage(target, level, format, type, pixels);
}
inline void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetTexLevelParameterfv(target, level, pname, params);
}
inline void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetTexLevelParameteriv(target, level, pname, params);
}
inline void glGetTexParameterIiv(GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetTexParameterIiv(target, pname, params);
}
inline void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params) noexcept
{
        opengl_functions::glGetTexParameterIuiv(target, pname, params);
}
inline void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetTexParameterfv(target, pname, params);
}
inline void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetTexParameteriv(target, pname, params);
}
inline GLuint64 glGetTextureHandleARB(GLuint texture) noexcept
{
        return opengl_functions::glGetTextureHandleARB(texture);
}
inline GLuint64 glGetTextureHandleNV(GLuint texture) noexcept
{
        return opengl_functions::glGetTextureHandleNV(texture);
}
inline void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) noexcept
{
        opengl_functions::glGetTextureImage(texture, level, format, type, bufSize, pixels);
}
inline void glGetTextureImageEXT(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels) noexcept
{
        opengl_functions::glGetTextureImageEXT(texture, target, level, format, type, pixels);
}
inline void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetTextureLevelParameterfv(texture, level, pname, params);
}
inline void glGetTextureLevelParameterfvEXT(GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetTextureLevelParameterfvEXT(texture, target, level, pname, params);
}
inline void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetTextureLevelParameteriv(texture, level, pname, params);
}
inline void glGetTextureLevelParameterivEXT(GLuint texture, GLenum target, GLint level, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetTextureLevelParameterivEXT(texture, target, level, pname, params);
}
inline void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetTextureParameterIiv(texture, pname, params);
}
inline void glGetTextureParameterIivEXT(GLuint texture, GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetTextureParameterIivEXT(texture, target, pname, params);
}
inline void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint *params) noexcept
{
        opengl_functions::glGetTextureParameterIuiv(texture, pname, params);
}
inline void glGetTextureParameterIuivEXT(GLuint texture, GLenum target, GLenum pname, GLuint *params) noexcept
{
        opengl_functions::glGetTextureParameterIuivEXT(texture, target, pname, params);
}
inline void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetTextureParameterfv(texture, pname, params);
}
inline void glGetTextureParameterfvEXT(GLuint texture, GLenum target, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetTextureParameterfvEXT(texture, target, pname, params);
}
inline void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetTextureParameteriv(texture, pname, params);
}
inline void glGetTextureParameterivEXT(GLuint texture, GLenum target, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetTextureParameterivEXT(texture, target, pname, params);
}
inline GLuint64 glGetTextureSamplerHandleARB(GLuint texture, GLuint sampler) noexcept
{
        return opengl_functions::glGetTextureSamplerHandleARB(texture, sampler);
}
inline GLuint64 glGetTextureSamplerHandleNV(GLuint texture, GLuint sampler) noexcept
{
        return opengl_functions::glGetTextureSamplerHandleNV(texture, sampler);
}
inline void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels) noexcept
{
        opengl_functions::glGetTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, pixels);
}
inline void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) noexcept
{
        opengl_functions::glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
}
inline void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64 *param) noexcept
{
        opengl_functions::glGetTransformFeedbacki64_v(xfb, pname, index, param);
}
inline void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint *param) noexcept
{
        opengl_functions::glGetTransformFeedbacki_v(xfb, pname, index, param);
}
inline void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint *param) noexcept
{
        opengl_functions::glGetTransformFeedbackiv(xfb, pname, param);
}
inline GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName) noexcept
{
        return opengl_functions::glGetUniformBlockIndex(program, uniformBlockName);
}
inline void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices) noexcept
{
        opengl_functions::glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
}
inline GLint glGetUniformLocation(GLuint program, const GLchar *name) noexcept
{
        return opengl_functions::glGetUniformLocation(program, name);
}
inline void glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint *params) noexcept
{
        opengl_functions::glGetUniformSubroutineuiv(shadertype, location, params);
}
inline void glGetUniformdv(GLuint program, GLint location, GLdouble *params) noexcept
{
        opengl_functions::glGetUniformdv(program, location, params);
}
inline void glGetUniformfv(GLuint program, GLint location, GLfloat *params) noexcept
{
        opengl_functions::glGetUniformfv(program, location, params);
}
inline void glGetUniformi64vARB(GLuint program, GLint location, GLint64 *params) noexcept
{
        opengl_functions::glGetUniformi64vARB(program, location, params);
}
inline void glGetUniformi64vNV(GLuint program, GLint location, GLint64EXT *params) noexcept
{
        opengl_functions::glGetUniformi64vNV(program, location, params);
}
inline void glGetUniformiv(GLuint program, GLint location, GLint *params) noexcept
{
        opengl_functions::glGetUniformiv(program, location, params);
}
inline void glGetUniformui64vARB(GLuint program, GLint location, GLuint64 *params) noexcept
{
        opengl_functions::glGetUniformui64vARB(program, location, params);
}
inline void glGetUniformui64vNV(GLuint program, GLint location, GLuint64EXT *params) noexcept
{
        opengl_functions::glGetUniformui64vNV(program, location, params);
}
inline void glGetUniformuiv(GLuint program, GLint location, GLuint *params) noexcept
{
        opengl_functions::glGetUniformuiv(program, location, params);
}
inline void glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64 *param) noexcept
{
        opengl_functions::glGetVertexArrayIndexed64iv(vaobj, index, pname, param);
}
inline void glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint *param) noexcept
{
        opengl_functions::glGetVertexArrayIndexediv(vaobj, index, pname, param);
}
inline void glGetVertexArrayIntegeri_vEXT(GLuint vaobj, GLuint index, GLenum pname, GLint *param) noexcept
{
        opengl_functions::glGetVertexArrayIntegeri_vEXT(vaobj, index, pname, param);
}
inline void glGetVertexArrayIntegervEXT(GLuint vaobj, GLenum pname, GLint *param) noexcept
{
        opengl_functions::glGetVertexArrayIntegervEXT(vaobj, pname, param);
}
inline void glGetVertexArrayPointeri_vEXT(GLuint vaobj, GLuint index, GLenum pname, void **param) noexcept
{
        opengl_functions::glGetVertexArrayPointeri_vEXT(vaobj, index, pname, param);
}
inline void glGetVertexArrayPointervEXT(GLuint vaobj, GLenum pname, void **param) noexcept
{
        opengl_functions::glGetVertexArrayPointervEXT(vaobj, pname, param);
}
inline void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint *param) noexcept
{
        opengl_functions::glGetVertexArrayiv(vaobj, pname, param);
}
inline void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetVertexAttribIiv(index, pname, params);
}
inline void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params) noexcept
{
        opengl_functions::glGetVertexAttribIuiv(index, pname, params);
}
inline void glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble *params) noexcept
{
        opengl_functions::glGetVertexAttribLdv(index, pname, params);
}
inline void glGetVertexAttribLi64vNV(GLuint index, GLenum pname, GLint64EXT *params) noexcept
{
        opengl_functions::glGetVertexAttribLi64vNV(index, pname, params);
}
inline void glGetVertexAttribLui64vARB(GLuint index, GLenum pname, GLuint64EXT *params) noexcept
{
        opengl_functions::glGetVertexAttribLui64vARB(index, pname, params);
}
inline void glGetVertexAttribLui64vNV(GLuint index, GLenum pname, GLuint64EXT *params) noexcept
{
        opengl_functions::glGetVertexAttribLui64vNV(index, pname, params);
}
inline void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer) noexcept
{
        opengl_functions::glGetVertexAttribPointerv(index, pname, pointer);
}
inline void glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble *params) noexcept
{
        opengl_functions::glGetVertexAttribdv(index, pname, params);
}
inline void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params) noexcept
{
        opengl_functions::glGetVertexAttribfv(index, pname, params);
}
inline void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params) noexcept
{
        opengl_functions::glGetVertexAttribiv(index, pname, params);
}
inline GLVULKANPROCNV glGetVkProcAddrNV(const GLchar *name) noexcept
{
        return opengl_functions::glGetVkProcAddrNV(name);
}
inline void glGetnCompressedTexImage(GLenum target, GLint lod, GLsizei bufSize, void *pixels) noexcept
{
        opengl_functions::glGetnCompressedTexImage(target, lod, bufSize, pixels);
}
inline void glGetnCompressedTexImageARB(GLenum target, GLint lod, GLsizei bufSize, void *img) noexcept
{
        opengl_functions::glGetnCompressedTexImageARB(target, lod, bufSize, img);
}
inline void glGetnTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) noexcept
{
        opengl_functions::glGetnTexImage(target, level, format, type, bufSize, pixels);
}
inline void glGetnTexImageARB(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *img) noexcept
{
        opengl_functions::glGetnTexImageARB(target, level, format, type, bufSize, img);
}
inline void glGetnUniformdv(GLuint program, GLint location, GLsizei bufSize, GLdouble *params) noexcept
{
        opengl_functions::glGetnUniformdv(program, location, bufSize, params);
}
inline void glGetnUniformdvARB(GLuint program, GLint location, GLsizei bufSize, GLdouble *params) noexcept
{
        opengl_functions::glGetnUniformdvARB(program, location, bufSize, params);
}
inline void glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat *params) noexcept
{
        opengl_functions::glGetnUniformfv(program, location, bufSize, params);
}
inline void glGetnUniformfvARB(GLuint program, GLint location, GLsizei bufSize, GLfloat *params) noexcept
{
        opengl_functions::glGetnUniformfvARB(program, location, bufSize, params);
}
inline void glGetnUniformi64vARB(GLuint program, GLint location, GLsizei bufSize, GLint64 *params) noexcept
{
        opengl_functions::glGetnUniformi64vARB(program, location, bufSize, params);
}
inline void glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params) noexcept
{
        opengl_functions::glGetnUniformiv(program, location, bufSize, params);
}
inline void glGetnUniformivARB(GLuint program, GLint location, GLsizei bufSize, GLint *params) noexcept
{
        opengl_functions::glGetnUniformivARB(program, location, bufSize, params);
}
inline void glGetnUniformui64vARB(GLuint program, GLint location, GLsizei bufSize, GLuint64 *params) noexcept
{
        opengl_functions::glGetnUniformui64vARB(program, location, bufSize, params);
}
inline void glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint *params) noexcept
{
        opengl_functions::glGetnUniformuiv(program, location, bufSize, params);
}
inline void glGetnUniformuivARB(GLuint program, GLint location, GLsizei bufSize, GLuint *params) noexcept
{
        opengl_functions::glGetnUniformuivARB(program, location, bufSize, params);
}
inline void glHint(GLenum target, GLenum mode) noexcept
{
        opengl_functions::glHint(target, mode);
}
inline void glIndexFormatNV(GLenum type, GLsizei stride) noexcept
{
        opengl_functions::glIndexFormatNV(type, stride);
}
inline void glInsertEventMarkerEXT(GLsizei length, const GLchar *marker) noexcept
{
        opengl_functions::glInsertEventMarkerEXT(length, marker);
}
inline void glInterpolatePathsNV(GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight) noexcept
{
        opengl_functions::glInterpolatePathsNV(resultPath, pathA, pathB, weight);
}
inline void glInvalidateBufferData(GLuint buffer) noexcept
{
        opengl_functions::glInvalidateBufferData(buffer);
}
inline void glInvalidateBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length) noexcept
{
        opengl_functions::glInvalidateBufferSubData(buffer, offset, length);
}
inline void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments) noexcept
{
        opengl_functions::glInvalidateFramebuffer(target, numAttachments, attachments);
}
inline void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments) noexcept
{
        opengl_functions::glInvalidateNamedFramebufferData(framebuffer, numAttachments, attachments);
}
inline void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glInvalidateNamedFramebufferSubData(framebuffer, numAttachments, attachments, x, y, width, height);
}
inline void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
}
inline void glInvalidateTexImage(GLuint texture, GLint level) noexcept
{
        opengl_functions::glInvalidateTexImage(texture, level);
}
inline void glInvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth) noexcept
{
        opengl_functions::glInvalidateTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth);
}
inline GLboolean glIsBuffer(GLuint buffer) noexcept
{
        return opengl_functions::glIsBuffer(buffer);
}
inline GLboolean glIsBufferResidentNV(GLenum target) noexcept
{
        return opengl_functions::glIsBufferResidentNV(target);
}
inline GLboolean glIsCommandListNV(GLuint list) noexcept
{
        return opengl_functions::glIsCommandListNV(list);
}
inline GLboolean glIsEnabled(GLenum cap) noexcept
{
        return opengl_functions::glIsEnabled(cap);
}
inline GLboolean glIsEnabledIndexedEXT(GLenum target, GLuint index) noexcept
{
        return opengl_functions::glIsEnabledIndexedEXT(target, index);
}
inline GLboolean glIsEnabledi(GLenum target, GLuint index) noexcept
{
        return opengl_functions::glIsEnabledi(target, index);
}
inline GLboolean glIsFramebuffer(GLuint framebuffer) noexcept
{
        return opengl_functions::glIsFramebuffer(framebuffer);
}
inline GLboolean glIsImageHandleResidentARB(GLuint64 handle) noexcept
{
        return opengl_functions::glIsImageHandleResidentARB(handle);
}
inline GLboolean glIsImageHandleResidentNV(GLuint64 handle) noexcept
{
        return opengl_functions::glIsImageHandleResidentNV(handle);
}
inline GLboolean glIsNamedBufferResidentNV(GLuint buffer) noexcept
{
        return opengl_functions::glIsNamedBufferResidentNV(buffer);
}
inline GLboolean glIsNamedStringARB(GLint namelen, const GLchar *name) noexcept
{
        return opengl_functions::glIsNamedStringARB(namelen, name);
}
inline GLboolean glIsPathNV(GLuint path) noexcept
{
        return opengl_functions::glIsPathNV(path);
}
inline GLboolean glIsPointInFillPathNV(GLuint path, GLuint mask, GLfloat x, GLfloat y) noexcept
{
        return opengl_functions::glIsPointInFillPathNV(path, mask, x, y);
}
inline GLboolean glIsPointInStrokePathNV(GLuint path, GLfloat x, GLfloat y) noexcept
{
        return opengl_functions::glIsPointInStrokePathNV(path, x, y);
}
inline GLboolean glIsProgram(GLuint program) noexcept
{
        return opengl_functions::glIsProgram(program);
}
inline GLboolean glIsProgramPipeline(GLuint pipeline) noexcept
{
        return opengl_functions::glIsProgramPipeline(pipeline);
}
inline GLboolean glIsQuery(GLuint id) noexcept
{
        return opengl_functions::glIsQuery(id);
}
inline GLboolean glIsRenderbuffer(GLuint renderbuffer) noexcept
{
        return opengl_functions::glIsRenderbuffer(renderbuffer);
}
inline GLboolean glIsSampler(GLuint sampler) noexcept
{
        return opengl_functions::glIsSampler(sampler);
}
inline GLboolean glIsShader(GLuint shader) noexcept
{
        return opengl_functions::glIsShader(shader);
}
inline GLboolean glIsStateNV(GLuint state) noexcept
{
        return opengl_functions::glIsStateNV(state);
}
inline GLboolean glIsSync(GLsync sync) noexcept
{
        return opengl_functions::glIsSync(sync);
}
inline GLboolean glIsTexture(GLuint texture) noexcept
{
        return opengl_functions::glIsTexture(texture);
}
inline GLboolean glIsTextureHandleResidentARB(GLuint64 handle) noexcept
{
        return opengl_functions::glIsTextureHandleResidentARB(handle);
}
inline GLboolean glIsTextureHandleResidentNV(GLuint64 handle) noexcept
{
        return opengl_functions::glIsTextureHandleResidentNV(handle);
}
inline GLboolean glIsTransformFeedback(GLuint id) noexcept
{
        return opengl_functions::glIsTransformFeedback(id);
}
inline GLboolean glIsVertexArray(GLuint array) noexcept
{
        return opengl_functions::glIsVertexArray(array);
}
inline void glLabelObjectEXT(GLenum type, GLuint object, GLsizei length, const GLchar *label) noexcept
{
        opengl_functions::glLabelObjectEXT(type, object, length, label);
}
inline void glLineWidth(GLfloat width) noexcept
{
        opengl_functions::glLineWidth(width);
}
inline void glLinkProgram(GLuint program) noexcept
{
        opengl_functions::glLinkProgram(program);
}
inline void glListDrawCommandsStatesClientNV(GLuint list, GLuint segment, const void **indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count) noexcept
{
        opengl_functions::glListDrawCommandsStatesClientNV(list, segment, indirects, sizes, states, fbos, count);
}
inline void glLogicOp(GLenum opcode) noexcept
{
        opengl_functions::glLogicOp(opcode);
}
inline void glMakeBufferNonResidentNV(GLenum target) noexcept
{
        opengl_functions::glMakeBufferNonResidentNV(target);
}
inline void glMakeBufferResidentNV(GLenum target, GLenum access) noexcept
{
        opengl_functions::glMakeBufferResidentNV(target, access);
}
inline void glMakeImageHandleNonResidentARB(GLuint64 handle) noexcept
{
        opengl_functions::glMakeImageHandleNonResidentARB(handle);
}
inline void glMakeImageHandleNonResidentNV(GLuint64 handle) noexcept
{
        opengl_functions::glMakeImageHandleNonResidentNV(handle);
}
inline void glMakeImageHandleResidentARB(GLuint64 handle, GLenum access) noexcept
{
        opengl_functions::glMakeImageHandleResidentARB(handle, access);
}
inline void glMakeImageHandleResidentNV(GLuint64 handle, GLenum access) noexcept
{
        opengl_functions::glMakeImageHandleResidentNV(handle, access);
}
inline void glMakeNamedBufferNonResidentNV(GLuint buffer) noexcept
{
        opengl_functions::glMakeNamedBufferNonResidentNV(buffer);
}
inline void glMakeNamedBufferResidentNV(GLuint buffer, GLenum access) noexcept
{
        opengl_functions::glMakeNamedBufferResidentNV(buffer, access);
}
inline void glMakeTextureHandleNonResidentARB(GLuint64 handle) noexcept
{
        opengl_functions::glMakeTextureHandleNonResidentARB(handle);
}
inline void glMakeTextureHandleNonResidentNV(GLuint64 handle) noexcept
{
        opengl_functions::glMakeTextureHandleNonResidentNV(handle);
}
inline void glMakeTextureHandleResidentARB(GLuint64 handle) noexcept
{
        opengl_functions::glMakeTextureHandleResidentARB(handle);
}
inline void glMakeTextureHandleResidentNV(GLuint64 handle) noexcept
{
        opengl_functions::glMakeTextureHandleResidentNV(handle);
}
inline void * glMapBuffer(GLenum target, GLenum access) noexcept
{
        return opengl_functions::glMapBuffer(target, access);
}
inline void * glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) noexcept
{
        return opengl_functions::glMapBufferRange(target, offset, length, access);
}
inline void * glMapNamedBuffer(GLuint buffer, GLenum access) noexcept
{
        return opengl_functions::glMapNamedBuffer(buffer, access);
}
inline void * glMapNamedBufferEXT(GLuint buffer, GLenum access) noexcept
{
        return opengl_functions::glMapNamedBufferEXT(buffer, access);
}
inline void * glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) noexcept
{
        return opengl_functions::glMapNamedBufferRange(buffer, offset, length, access);
}
inline void * glMapNamedBufferRangeEXT(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) noexcept
{
        return opengl_functions::glMapNamedBufferRangeEXT(buffer, offset, length, access);
}
inline void glMatrixFrustumEXT(GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) noexcept
{
        opengl_functions::glMatrixFrustumEXT(mode, left, right, bottom, top, zNear, zFar);
}
inline void glMatrixLoad3x2fNV(GLenum matrixMode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixLoad3x2fNV(matrixMode, m);
}
inline void glMatrixLoad3x3fNV(GLenum matrixMode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixLoad3x3fNV(matrixMode, m);
}
inline void glMatrixLoadIdentityEXT(GLenum mode) noexcept
{
        opengl_functions::glMatrixLoadIdentityEXT(mode);
}
inline void glMatrixLoadTranspose3x3fNV(GLenum matrixMode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixLoadTranspose3x3fNV(matrixMode, m);
}
inline void glMatrixLoadTransposedEXT(GLenum mode, const GLdouble *m) noexcept
{
        opengl_functions::glMatrixLoadTransposedEXT(mode, m);
}
inline void glMatrixLoadTransposefEXT(GLenum mode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixLoadTransposefEXT(mode, m);
}
inline void glMatrixLoaddEXT(GLenum mode, const GLdouble *m) noexcept
{
        opengl_functions::glMatrixLoaddEXT(mode, m);
}
inline void glMatrixLoadfEXT(GLenum mode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixLoadfEXT(mode, m);
}
inline void glMatrixMult3x2fNV(GLenum matrixMode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixMult3x2fNV(matrixMode, m);
}
inline void glMatrixMult3x3fNV(GLenum matrixMode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixMult3x3fNV(matrixMode, m);
}
inline void glMatrixMultTranspose3x3fNV(GLenum matrixMode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixMultTranspose3x3fNV(matrixMode, m);
}
inline void glMatrixMultTransposedEXT(GLenum mode, const GLdouble *m) noexcept
{
        opengl_functions::glMatrixMultTransposedEXT(mode, m);
}
inline void glMatrixMultTransposefEXT(GLenum mode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixMultTransposefEXT(mode, m);
}
inline void glMatrixMultdEXT(GLenum mode, const GLdouble *m) noexcept
{
        opengl_functions::glMatrixMultdEXT(mode, m);
}
inline void glMatrixMultfEXT(GLenum mode, const GLfloat *m) noexcept
{
        opengl_functions::glMatrixMultfEXT(mode, m);
}
inline void glMatrixOrthoEXT(GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) noexcept
{
        opengl_functions::glMatrixOrthoEXT(mode, left, right, bottom, top, zNear, zFar);
}
inline void glMatrixPopEXT(GLenum mode) noexcept
{
        opengl_functions::glMatrixPopEXT(mode);
}
inline void glMatrixPushEXT(GLenum mode) noexcept
{
        opengl_functions::glMatrixPushEXT(mode);
}
inline void glMatrixRotatedEXT(GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z) noexcept
{
        opengl_functions::glMatrixRotatedEXT(mode, angle, x, y, z);
}
inline void glMatrixRotatefEXT(GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z) noexcept
{
        opengl_functions::glMatrixRotatefEXT(mode, angle, x, y, z);
}
inline void glMatrixScaledEXT(GLenum mode, GLdouble x, GLdouble y, GLdouble z) noexcept
{
        opengl_functions::glMatrixScaledEXT(mode, x, y, z);
}
inline void glMatrixScalefEXT(GLenum mode, GLfloat x, GLfloat y, GLfloat z) noexcept
{
        opengl_functions::glMatrixScalefEXT(mode, x, y, z);
}
inline void glMatrixTranslatedEXT(GLenum mode, GLdouble x, GLdouble y, GLdouble z) noexcept
{
        opengl_functions::glMatrixTranslatedEXT(mode, x, y, z);
}
inline void glMatrixTranslatefEXT(GLenum mode, GLfloat x, GLfloat y, GLfloat z) noexcept
{
        opengl_functions::glMatrixTranslatefEXT(mode, x, y, z);
}
inline void glMaxShaderCompilerThreadsARB(GLuint count) noexcept
{
        opengl_functions::glMaxShaderCompilerThreadsARB(count);
}
inline void glMaxShaderCompilerThreadsKHR(GLuint count) noexcept
{
        opengl_functions::glMaxShaderCompilerThreadsKHR(count);
}
inline void glMemoryBarrier(GLbitfield barriers) noexcept
{
        opengl_functions::glMemoryBarrier(barriers);
}
inline void glMemoryBarrierByRegion(GLbitfield barriers) noexcept
{
        opengl_functions::glMemoryBarrierByRegion(barriers);
}
inline void glMinSampleShading(GLfloat value) noexcept
{
        opengl_functions::glMinSampleShading(value);
}
inline void glMinSampleShadingARB(GLfloat value) noexcept
{
        opengl_functions::glMinSampleShadingARB(value);
}
inline void glMultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount) noexcept
{
        opengl_functions::glMultiDrawArrays(mode, first, count, drawcount);
}
inline void glMultiDrawArraysIndirect(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride) noexcept
{
        opengl_functions::glMultiDrawArraysIndirect(mode, indirect, drawcount, stride);
}
inline void glMultiDrawArraysIndirectBindlessCountNV(GLenum mode, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount) noexcept
{
        opengl_functions::glMultiDrawArraysIndirectBindlessCountNV(mode, indirect, drawCount, maxDrawCount, stride, vertexBufferCount);
}
inline void glMultiDrawArraysIndirectBindlessNV(GLenum mode, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount) noexcept
{
        opengl_functions::glMultiDrawArraysIndirectBindlessNV(mode, indirect, drawCount, stride, vertexBufferCount);
}
inline void glMultiDrawArraysIndirectCount(GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) noexcept
{
        opengl_functions::glMultiDrawArraysIndirectCount(mode, indirect, drawcount, maxdrawcount, stride);
}
inline void glMultiDrawArraysIndirectCountARB(GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) noexcept
{
        opengl_functions::glMultiDrawArraysIndirectCountARB(mode, indirect, drawcount, maxdrawcount, stride);
}
inline void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount) noexcept
{
        opengl_functions::glMultiDrawElements(mode, count, type, indices, drawcount);
}
inline void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex) noexcept
{
        opengl_functions::glMultiDrawElementsBaseVertex(mode, count, type, indices, drawcount, basevertex);
}
inline void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride) noexcept
{
        opengl_functions::glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride);
}
inline void glMultiDrawElementsIndirectBindlessCountNV(GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount) noexcept
{
        opengl_functions::glMultiDrawElementsIndirectBindlessCountNV(mode, type, indirect, drawCount, maxDrawCount, stride, vertexBufferCount);
}
inline void glMultiDrawElementsIndirectBindlessNV(GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount) noexcept
{
        opengl_functions::glMultiDrawElementsIndirectBindlessNV(mode, type, indirect, drawCount, stride, vertexBufferCount);
}
inline void glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) noexcept
{
        opengl_functions::glMultiDrawElementsIndirectCount(mode, type, indirect, drawcount, maxdrawcount, stride);
}
inline void glMultiDrawElementsIndirectCountARB(GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) noexcept
{
        opengl_functions::glMultiDrawElementsIndirectCountARB(mode, type, indirect, drawcount, maxdrawcount, stride);
}
inline void glMultiDrawMeshTasksIndirectCountNV(GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) noexcept
{
        opengl_functions::glMultiDrawMeshTasksIndirectCountNV(indirect, drawcount, maxdrawcount, stride);
}
inline void glMultiDrawMeshTasksIndirectNV(GLintptr indirect, GLsizei drawcount, GLsizei stride) noexcept
{
        opengl_functions::glMultiDrawMeshTasksIndirectNV(indirect, drawcount, stride);
}
inline void glMultiTexBufferEXT(GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer) noexcept
{
        opengl_functions::glMultiTexBufferEXT(texunit, target, internalformat, buffer);
}
inline void glMultiTexCoordPointerEXT(GLenum texunit, GLint size, GLenum type, GLsizei stride, const void *pointer) noexcept
{
        opengl_functions::glMultiTexCoordPointerEXT(texunit, size, type, stride, pointer);
}
inline void glMultiTexEnvfEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat param) noexcept
{
        opengl_functions::glMultiTexEnvfEXT(texunit, target, pname, param);
}
inline void glMultiTexEnvfvEXT(GLenum texunit, GLenum target, GLenum pname, const GLfloat *params) noexcept
{
        opengl_functions::glMultiTexEnvfvEXT(texunit, target, pname, params);
}
inline void glMultiTexEnviEXT(GLenum texunit, GLenum target, GLenum pname, GLint param) noexcept
{
        opengl_functions::glMultiTexEnviEXT(texunit, target, pname, param);
}
inline void glMultiTexEnvivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glMultiTexEnvivEXT(texunit, target, pname, params);
}
inline void glMultiTexGendEXT(GLenum texunit, GLenum coord, GLenum pname, GLdouble param) noexcept
{
        opengl_functions::glMultiTexGendEXT(texunit, coord, pname, param);
}
inline void glMultiTexGendvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLdouble *params) noexcept
{
        opengl_functions::glMultiTexGendvEXT(texunit, coord, pname, params);
}
inline void glMultiTexGenfEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat param) noexcept
{
        opengl_functions::glMultiTexGenfEXT(texunit, coord, pname, param);
}
inline void glMultiTexGenfvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLfloat *params) noexcept
{
        opengl_functions::glMultiTexGenfvEXT(texunit, coord, pname, params);
}
inline void glMultiTexGeniEXT(GLenum texunit, GLenum coord, GLenum pname, GLint param) noexcept
{
        opengl_functions::glMultiTexGeniEXT(texunit, coord, pname, param);
}
inline void glMultiTexGenivEXT(GLenum texunit, GLenum coord, GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glMultiTexGenivEXT(texunit, coord, pname, params);
}
inline void glMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, format, type, pixels);
}
inline void glMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, format, type, pixels);
}
inline void glMultiTexImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, format, type, pixels);
}
inline void glMultiTexParameterIivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glMultiTexParameterIivEXT(texunit, target, pname, params);
}
inline void glMultiTexParameterIuivEXT(GLenum texunit, GLenum target, GLenum pname, const GLuint *params) noexcept
{
        opengl_functions::glMultiTexParameterIuivEXT(texunit, target, pname, params);
}
inline void glMultiTexParameterfEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat param) noexcept
{
        opengl_functions::glMultiTexParameterfEXT(texunit, target, pname, param);
}
inline void glMultiTexParameterfvEXT(GLenum texunit, GLenum target, GLenum pname, const GLfloat *params) noexcept
{
        opengl_functions::glMultiTexParameterfvEXT(texunit, target, pname, params);
}
inline void glMultiTexParameteriEXT(GLenum texunit, GLenum target, GLenum pname, GLint param) noexcept
{
        opengl_functions::glMultiTexParameteriEXT(texunit, target, pname, param);
}
inline void glMultiTexParameterivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glMultiTexParameterivEXT(texunit, target, pname, params);
}
inline void glMultiTexRenderbufferEXT(GLenum texunit, GLenum target, GLuint renderbuffer) noexcept
{
        opengl_functions::glMultiTexRenderbufferEXT(texunit, target, renderbuffer);
}
inline void glMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, type, pixels);
}
inline void glMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, type, pixels);
}
inline void glMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
inline void glNamedBufferAttachMemoryNV(GLuint buffer, GLuint memory, GLuint64 offset) noexcept
{
        opengl_functions::glNamedBufferAttachMemoryNV(buffer, memory, offset);
}
inline void glNamedBufferData(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage) noexcept
{
        opengl_functions::glNamedBufferData(buffer, size, data, usage);
}
inline void glNamedBufferDataEXT(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage) noexcept
{
        opengl_functions::glNamedBufferDataEXT(buffer, size, data, usage);
}
inline void glNamedBufferPageCommitmentARB(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit) noexcept
{
        opengl_functions::glNamedBufferPageCommitmentARB(buffer, offset, size, commit);
}
inline void glNamedBufferPageCommitmentEXT(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit) noexcept
{
        opengl_functions::glNamedBufferPageCommitmentEXT(buffer, offset, size, commit);
}
inline void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags) noexcept
{
        opengl_functions::glNamedBufferStorage(buffer, size, data, flags);
}
inline void glNamedBufferStorageEXT(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags) noexcept
{
        opengl_functions::glNamedBufferStorageEXT(buffer, size, data, flags);
}
inline void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data) noexcept
{
        opengl_functions::glNamedBufferSubData(buffer, offset, size, data);
}
inline void glNamedBufferSubDataEXT(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data) noexcept
{
        opengl_functions::glNamedBufferSubDataEXT(buffer, offset, size, data);
}
inline void glNamedCopyBufferSubDataEXT(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) noexcept
{
        opengl_functions::glNamedCopyBufferSubDataEXT(readBuffer, writeBuffer, readOffset, writeOffset, size);
}
inline void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf) noexcept
{
        opengl_functions::glNamedFramebufferDrawBuffer(framebuffer, buf);
}
inline void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum *bufs) noexcept
{
        opengl_functions::glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
}
inline void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param) noexcept
{
        opengl_functions::glNamedFramebufferParameteri(framebuffer, pname, param);
}
inline void glNamedFramebufferParameteriEXT(GLuint framebuffer, GLenum pname, GLint param) noexcept
{
        opengl_functions::glNamedFramebufferParameteriEXT(framebuffer, pname, param);
}
inline void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src) noexcept
{
        opengl_functions::glNamedFramebufferReadBuffer(framebuffer, src);
}
inline void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) noexcept
{
        opengl_functions::glNamedFramebufferRenderbuffer(framebuffer, attachment, renderbuffertarget, renderbuffer);
}
inline void glNamedFramebufferRenderbufferEXT(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) noexcept
{
        opengl_functions::glNamedFramebufferRenderbufferEXT(framebuffer, attachment, renderbuffertarget, renderbuffer);
}
inline void glNamedFramebufferSampleLocationsfvARB(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v) noexcept
{
        opengl_functions::glNamedFramebufferSampleLocationsfvARB(framebuffer, start, count, v);
}
inline void glNamedFramebufferSampleLocationsfvNV(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v) noexcept
{
        opengl_functions::glNamedFramebufferSampleLocationsfvNV(framebuffer, start, count, v);
}
inline void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) noexcept
{
        opengl_functions::glNamedFramebufferTexture(framebuffer, attachment, texture, level);
}
inline void glNamedFramebufferTexture1DEXT(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) noexcept
{
        opengl_functions::glNamedFramebufferTexture1DEXT(framebuffer, attachment, textarget, texture, level);
}
inline void glNamedFramebufferTexture2DEXT(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) noexcept
{
        opengl_functions::glNamedFramebufferTexture2DEXT(framebuffer, attachment, textarget, texture, level);
}
inline void glNamedFramebufferTexture3DEXT(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) noexcept
{
        opengl_functions::glNamedFramebufferTexture3DEXT(framebuffer, attachment, textarget, texture, level, zoffset);
}
inline void glNamedFramebufferTextureEXT(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) noexcept
{
        opengl_functions::glNamedFramebufferTextureEXT(framebuffer, attachment, texture, level);
}
inline void glNamedFramebufferTextureFaceEXT(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face) noexcept
{
        opengl_functions::glNamedFramebufferTextureFaceEXT(framebuffer, attachment, texture, level, face);
}
inline void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) noexcept
{
        opengl_functions::glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, layer);
}
inline void glNamedFramebufferTextureLayerEXT(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) noexcept
{
        opengl_functions::glNamedFramebufferTextureLayerEXT(framebuffer, attachment, texture, level, layer);
}
inline void glNamedProgramLocalParameter4dEXT(GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) noexcept
{
        opengl_functions::glNamedProgramLocalParameter4dEXT(program, target, index, x, y, z, w);
}
inline void glNamedProgramLocalParameter4dvEXT(GLuint program, GLenum target, GLuint index, const GLdouble *params) noexcept
{
        opengl_functions::glNamedProgramLocalParameter4dvEXT(program, target, index, params);
}
inline void glNamedProgramLocalParameter4fEXT(GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) noexcept
{
        opengl_functions::glNamedProgramLocalParameter4fEXT(program, target, index, x, y, z, w);
}
inline void glNamedProgramLocalParameter4fvEXT(GLuint program, GLenum target, GLuint index, const GLfloat *params) noexcept
{
        opengl_functions::glNamedProgramLocalParameter4fvEXT(program, target, index, params);
}
inline void glNamedProgramLocalParameterI4iEXT(GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w) noexcept
{
        opengl_functions::glNamedProgramLocalParameterI4iEXT(program, target, index, x, y, z, w);
}
inline void glNamedProgramLocalParameterI4ivEXT(GLuint program, GLenum target, GLuint index, const GLint *params) noexcept
{
        opengl_functions::glNamedProgramLocalParameterI4ivEXT(program, target, index, params);
}
inline void glNamedProgramLocalParameterI4uiEXT(GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) noexcept
{
        opengl_functions::glNamedProgramLocalParameterI4uiEXT(program, target, index, x, y, z, w);
}
inline void glNamedProgramLocalParameterI4uivEXT(GLuint program, GLenum target, GLuint index, const GLuint *params) noexcept
{
        opengl_functions::glNamedProgramLocalParameterI4uivEXT(program, target, index, params);
}
inline void glNamedProgramLocalParameters4fvEXT(GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat *params) noexcept
{
        opengl_functions::glNamedProgramLocalParameters4fvEXT(program, target, index, count, params);
}
inline void glNamedProgramLocalParametersI4ivEXT(GLuint program, GLenum target, GLuint index, GLsizei count, const GLint *params) noexcept
{
        opengl_functions::glNamedProgramLocalParametersI4ivEXT(program, target, index, count, params);
}
inline void glNamedProgramLocalParametersI4uivEXT(GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint *params) noexcept
{
        opengl_functions::glNamedProgramLocalParametersI4uivEXT(program, target, index, count, params);
}
inline void glNamedProgramStringEXT(GLuint program, GLenum target, GLenum format, GLsizei len, const void *string) noexcept
{
        opengl_functions::glNamedProgramStringEXT(program, target, format, len, string);
}
inline void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glNamedRenderbufferStorage(renderbuffer, internalformat, width, height);
}
inline void glNamedRenderbufferStorageEXT(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glNamedRenderbufferStorageEXT(renderbuffer, internalformat, width, height);
}
inline void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glNamedRenderbufferStorageMultisample(renderbuffer, samples, internalformat, width, height);
}
inline void glNamedRenderbufferStorageMultisampleAdvancedAMD(GLuint renderbuffer, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glNamedRenderbufferStorageMultisampleAdvancedAMD(renderbuffer, samples, storageSamples, internalformat, width, height);
}
inline void glNamedRenderbufferStorageMultisampleCoverageEXT(GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glNamedRenderbufferStorageMultisampleCoverageEXT(renderbuffer, coverageSamples, colorSamples, internalformat, width, height);
}
inline void glNamedRenderbufferStorageMultisampleEXT(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glNamedRenderbufferStorageMultisampleEXT(renderbuffer, samples, internalformat, width, height);
}
inline void glNamedStringARB(GLenum type, GLint namelen, const GLchar *name, GLint stringlen, const GLchar *string) noexcept
{
        opengl_functions::glNamedStringARB(type, namelen, name, stringlen, string);
}
inline void glNormalFormatNV(GLenum type, GLsizei stride) noexcept
{
        opengl_functions::glNormalFormatNV(type, stride);
}
inline void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar *label) noexcept
{
        opengl_functions::glObjectLabel(identifier, name, length, label);
}
inline void glObjectPtrLabel(const void *ptr, GLsizei length, const GLchar *label) noexcept
{
        opengl_functions::glObjectPtrLabel(ptr, length, label);
}
inline void glPatchParameterfv(GLenum pname, const GLfloat *values) noexcept
{
        opengl_functions::glPatchParameterfv(pname, values);
}
inline void glPatchParameteri(GLenum pname, GLint value) noexcept
{
        opengl_functions::glPatchParameteri(pname, value);
}
inline void glPathCommandsNV(GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords) noexcept
{
        opengl_functions::glPathCommandsNV(path, numCommands, commands, numCoords, coordType, coords);
}
inline void glPathCoordsNV(GLuint path, GLsizei numCoords, GLenum coordType, const void *coords) noexcept
{
        opengl_functions::glPathCoordsNV(path, numCoords, coordType, coords);
}
inline void glPathCoverDepthFuncNV(GLenum func) noexcept
{
        opengl_functions::glPathCoverDepthFuncNV(func);
}
inline void glPathDashArrayNV(GLuint path, GLsizei dashCount, const GLfloat *dashArray) noexcept
{
        opengl_functions::glPathDashArrayNV(path, dashCount, dashArray);
}
inline GLenum glPathGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale) noexcept
{
        return opengl_functions::glPathGlyphIndexArrayNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale);
}
inline GLenum glPathGlyphIndexRangeNV(GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2]) noexcept
{
        return opengl_functions::glPathGlyphIndexRangeNV(fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount);
}
inline void glPathGlyphRangeNV(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale) noexcept
{
        opengl_functions::glPathGlyphRangeNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale);
}
inline void glPathGlyphsNV(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale) noexcept
{
        opengl_functions::glPathGlyphsNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale);
}
inline GLenum glPathMemoryGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale) noexcept
{
        return opengl_functions::glPathMemoryGlyphIndexArrayNV(firstPathName, fontTarget, fontSize, fontData, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale);
}
inline void glPathParameterfNV(GLuint path, GLenum pname, GLfloat value) noexcept
{
        opengl_functions::glPathParameterfNV(path, pname, value);
}
inline void glPathParameterfvNV(GLuint path, GLenum pname, const GLfloat *value) noexcept
{
        opengl_functions::glPathParameterfvNV(path, pname, value);
}
inline void glPathParameteriNV(GLuint path, GLenum pname, GLint value) noexcept
{
        opengl_functions::glPathParameteriNV(path, pname, value);
}
inline void glPathParameterivNV(GLuint path, GLenum pname, const GLint *value) noexcept
{
        opengl_functions::glPathParameterivNV(path, pname, value);
}
inline void glPathStencilDepthOffsetNV(GLfloat factor, GLfloat units) noexcept
{
        opengl_functions::glPathStencilDepthOffsetNV(factor, units);
}
inline void glPathStencilFuncNV(GLenum func, GLint ref, GLuint mask) noexcept
{
        opengl_functions::glPathStencilFuncNV(func, ref, mask);
}
inline void glPathStringNV(GLuint path, GLenum format, GLsizei length, const void *pathString) noexcept
{
        opengl_functions::glPathStringNV(path, format, length, pathString);
}
inline void glPathSubCommandsNV(GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords) noexcept
{
        opengl_functions::glPathSubCommandsNV(path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords);
}
inline void glPathSubCoordsNV(GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords) noexcept
{
        opengl_functions::glPathSubCoordsNV(path, coordStart, numCoords, coordType, coords);
}
inline void glPauseTransformFeedback() noexcept
{
        opengl_functions::glPauseTransformFeedback();
}
inline void glPixelStoref(GLenum pname, GLfloat param) noexcept
{
        opengl_functions::glPixelStoref(pname, param);
}
inline void glPixelStorei(GLenum pname, GLint param) noexcept
{
        opengl_functions::glPixelStorei(pname, param);
}
inline GLboolean glPointAlongPathNV(GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY) noexcept
{
        return opengl_functions::glPointAlongPathNV(path, startSegment, numSegments, distance, x, y, tangentX, tangentY);
}
inline void glPointParameterf(GLenum pname, GLfloat param) noexcept
{
        opengl_functions::glPointParameterf(pname, param);
}
inline void glPointParameterfv(GLenum pname, const GLfloat *params) noexcept
{
        opengl_functions::glPointParameterfv(pname, params);
}
inline void glPointParameteri(GLenum pname, GLint param) noexcept
{
        opengl_functions::glPointParameteri(pname, param);
}
inline void glPointParameteriv(GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glPointParameteriv(pname, params);
}
inline void glPointSize(GLfloat size) noexcept
{
        opengl_functions::glPointSize(size);
}
inline void glPolygonMode(GLenum face, GLenum mode) noexcept
{
        opengl_functions::glPolygonMode(face, mode);
}
inline void glPolygonOffset(GLfloat factor, GLfloat units) noexcept
{
        opengl_functions::glPolygonOffset(factor, units);
}
inline void glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp) noexcept
{
        opengl_functions::glPolygonOffsetClamp(factor, units, clamp);
}
inline void glPolygonOffsetClampEXT(GLfloat factor, GLfloat units, GLfloat clamp) noexcept
{
        opengl_functions::glPolygonOffsetClampEXT(factor, units, clamp);
}
inline void glPopDebugGroup() noexcept
{
        opengl_functions::glPopDebugGroup();
}
inline void glPopGroupMarkerEXT() noexcept
{
        opengl_functions::glPopGroupMarkerEXT();
}
inline void glPrimitiveBoundingBoxARB(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW) noexcept
{
        opengl_functions::glPrimitiveBoundingBoxARB(minX, minY, minZ, minW, maxX, maxY, maxZ, maxW);
}
inline void glPrimitiveRestartIndex(GLuint index) noexcept
{
        opengl_functions::glPrimitiveRestartIndex(index);
}
inline void glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length) noexcept
{
        opengl_functions::glProgramBinary(program, binaryFormat, binary, length);
}
inline void glProgramParameteri(GLuint program, GLenum pname, GLint value) noexcept
{
        opengl_functions::glProgramParameteri(program, pname, value);
}
inline void glProgramParameteriARB(GLuint program, GLenum pname, GLint value) noexcept
{
        opengl_functions::glProgramParameteriARB(program, pname, value);
}
inline void glProgramPathFragmentInputGenNV(GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs) noexcept
{
        opengl_functions::glProgramPathFragmentInputGenNV(program, location, genMode, components, coeffs);
}
inline void glProgramUniform1d(GLuint program, GLint location, GLdouble v0) noexcept
{
        opengl_functions::glProgramUniform1d(program, location, v0);
}
inline void glProgramUniform1dEXT(GLuint program, GLint location, GLdouble x) noexcept
{
        opengl_functions::glProgramUniform1dEXT(program, location, x);
}
inline void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniform1dv(program, location, count, value);
}
inline void glProgramUniform1dvEXT(GLuint program, GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniform1dvEXT(program, location, count, value);
}
inline void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) noexcept
{
        opengl_functions::glProgramUniform1f(program, location, v0);
}
inline void glProgramUniform1fEXT(GLuint program, GLint location, GLfloat v0) noexcept
{
        opengl_functions::glProgramUniform1fEXT(program, location, v0);
}
inline void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniform1fv(program, location, count, value);
}
inline void glProgramUniform1fvEXT(GLuint program, GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniform1fvEXT(program, location, count, value);
}
inline void glProgramUniform1i(GLuint program, GLint location, GLint v0) noexcept
{
        opengl_functions::glProgramUniform1i(program, location, v0);
}
inline void glProgramUniform1i64ARB(GLuint program, GLint location, GLint64 x) noexcept
{
        opengl_functions::glProgramUniform1i64ARB(program, location, x);
}
inline void glProgramUniform1i64NV(GLuint program, GLint location, GLint64EXT x) noexcept
{
        opengl_functions::glProgramUniform1i64NV(program, location, x);
}
inline void glProgramUniform1i64vARB(GLuint program, GLint location, GLsizei count, const GLint64 *value) noexcept
{
        opengl_functions::glProgramUniform1i64vARB(program, location, count, value);
}
inline void glProgramUniform1i64vNV(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) noexcept
{
        opengl_functions::glProgramUniform1i64vNV(program, location, count, value);
}
inline void glProgramUniform1iEXT(GLuint program, GLint location, GLint v0) noexcept
{
        opengl_functions::glProgramUniform1iEXT(program, location, v0);
}
inline void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glProgramUniform1iv(program, location, count, value);
}
inline void glProgramUniform1ivEXT(GLuint program, GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glProgramUniform1ivEXT(program, location, count, value);
}
inline void glProgramUniform1ui(GLuint program, GLint location, GLuint v0) noexcept
{
        opengl_functions::glProgramUniform1ui(program, location, v0);
}
inline void glProgramUniform1ui64ARB(GLuint program, GLint location, GLuint64 x) noexcept
{
        opengl_functions::glProgramUniform1ui64ARB(program, location, x);
}
inline void glProgramUniform1ui64NV(GLuint program, GLint location, GLuint64EXT x) noexcept
{
        opengl_functions::glProgramUniform1ui64NV(program, location, x);
}
inline void glProgramUniform1ui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glProgramUniform1ui64vARB(program, location, count, value);
}
inline void glProgramUniform1ui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glProgramUniform1ui64vNV(program, location, count, value);
}
inline void glProgramUniform1uiEXT(GLuint program, GLint location, GLuint v0) noexcept
{
        opengl_functions::glProgramUniform1uiEXT(program, location, v0);
}
inline void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glProgramUniform1uiv(program, location, count, value);
}
inline void glProgramUniform1uivEXT(GLuint program, GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glProgramUniform1uivEXT(program, location, count, value);
}
inline void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1) noexcept
{
        opengl_functions::glProgramUniform2d(program, location, v0, v1);
}
inline void glProgramUniform2dEXT(GLuint program, GLint location, GLdouble x, GLdouble y) noexcept
{
        opengl_functions::glProgramUniform2dEXT(program, location, x, y);
}
inline void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniform2dv(program, location, count, value);
}
inline void glProgramUniform2dvEXT(GLuint program, GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniform2dvEXT(program, location, count, value);
}
inline void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) noexcept
{
        opengl_functions::glProgramUniform2f(program, location, v0, v1);
}
inline void glProgramUniform2fEXT(GLuint program, GLint location, GLfloat v0, GLfloat v1) noexcept
{
        opengl_functions::glProgramUniform2fEXT(program, location, v0, v1);
}
inline void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniform2fv(program, location, count, value);
}
inline void glProgramUniform2fvEXT(GLuint program, GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniform2fvEXT(program, location, count, value);
}
inline void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) noexcept
{
        opengl_functions::glProgramUniform2i(program, location, v0, v1);
}
inline void glProgramUniform2i64ARB(GLuint program, GLint location, GLint64 x, GLint64 y) noexcept
{
        opengl_functions::glProgramUniform2i64ARB(program, location, x, y);
}
inline void glProgramUniform2i64NV(GLuint program, GLint location, GLint64EXT x, GLint64EXT y) noexcept
{
        opengl_functions::glProgramUniform2i64NV(program, location, x, y);
}
inline void glProgramUniform2i64vARB(GLuint program, GLint location, GLsizei count, const GLint64 *value) noexcept
{
        opengl_functions::glProgramUniform2i64vARB(program, location, count, value);
}
inline void glProgramUniform2i64vNV(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) noexcept
{
        opengl_functions::glProgramUniform2i64vNV(program, location, count, value);
}
inline void glProgramUniform2iEXT(GLuint program, GLint location, GLint v0, GLint v1) noexcept
{
        opengl_functions::glProgramUniform2iEXT(program, location, v0, v1);
}
inline void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glProgramUniform2iv(program, location, count, value);
}
inline void glProgramUniform2ivEXT(GLuint program, GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glProgramUniform2ivEXT(program, location, count, value);
}
inline void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1) noexcept
{
        opengl_functions::glProgramUniform2ui(program, location, v0, v1);
}
inline void glProgramUniform2ui64ARB(GLuint program, GLint location, GLuint64 x, GLuint64 y) noexcept
{
        opengl_functions::glProgramUniform2ui64ARB(program, location, x, y);
}
inline void glProgramUniform2ui64NV(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y) noexcept
{
        opengl_functions::glProgramUniform2ui64NV(program, location, x, y);
}
inline void glProgramUniform2ui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glProgramUniform2ui64vARB(program, location, count, value);
}
inline void glProgramUniform2ui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glProgramUniform2ui64vNV(program, location, count, value);
}
inline void glProgramUniform2uiEXT(GLuint program, GLint location, GLuint v0, GLuint v1) noexcept
{
        opengl_functions::glProgramUniform2uiEXT(program, location, v0, v1);
}
inline void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glProgramUniform2uiv(program, location, count, value);
}
inline void glProgramUniform2uivEXT(GLuint program, GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glProgramUniform2uivEXT(program, location, count, value);
}
inline void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2) noexcept
{
        opengl_functions::glProgramUniform3d(program, location, v0, v1, v2);
}
inline void glProgramUniform3dEXT(GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z) noexcept
{
        opengl_functions::glProgramUniform3dEXT(program, location, x, y, z);
}
inline void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniform3dv(program, location, count, value);
}
inline void glProgramUniform3dvEXT(GLuint program, GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniform3dvEXT(program, location, count, value);
}
inline void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) noexcept
{
        opengl_functions::glProgramUniform3f(program, location, v0, v1, v2);
}
inline void glProgramUniform3fEXT(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) noexcept
{
        opengl_functions::glProgramUniform3fEXT(program, location, v0, v1, v2);
}
inline void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniform3fv(program, location, count, value);
}
inline void glProgramUniform3fvEXT(GLuint program, GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniform3fvEXT(program, location, count, value);
}
inline void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) noexcept
{
        opengl_functions::glProgramUniform3i(program, location, v0, v1, v2);
}
inline void glProgramUniform3i64ARB(GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z) noexcept
{
        opengl_functions::glProgramUniform3i64ARB(program, location, x, y, z);
}
inline void glProgramUniform3i64NV(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z) noexcept
{
        opengl_functions::glProgramUniform3i64NV(program, location, x, y, z);
}
inline void glProgramUniform3i64vARB(GLuint program, GLint location, GLsizei count, const GLint64 *value) noexcept
{
        opengl_functions::glProgramUniform3i64vARB(program, location, count, value);
}
inline void glProgramUniform3i64vNV(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) noexcept
{
        opengl_functions::glProgramUniform3i64vNV(program, location, count, value);
}
inline void glProgramUniform3iEXT(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) noexcept
{
        opengl_functions::glProgramUniform3iEXT(program, location, v0, v1, v2);
}
inline void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glProgramUniform3iv(program, location, count, value);
}
inline void glProgramUniform3ivEXT(GLuint program, GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glProgramUniform3ivEXT(program, location, count, value);
}
inline void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) noexcept
{
        opengl_functions::glProgramUniform3ui(program, location, v0, v1, v2);
}
inline void glProgramUniform3ui64ARB(GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z) noexcept
{
        opengl_functions::glProgramUniform3ui64ARB(program, location, x, y, z);
}
inline void glProgramUniform3ui64NV(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z) noexcept
{
        opengl_functions::glProgramUniform3ui64NV(program, location, x, y, z);
}
inline void glProgramUniform3ui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glProgramUniform3ui64vARB(program, location, count, value);
}
inline void glProgramUniform3ui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glProgramUniform3ui64vNV(program, location, count, value);
}
inline void glProgramUniform3uiEXT(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) noexcept
{
        opengl_functions::glProgramUniform3uiEXT(program, location, v0, v1, v2);
}
inline void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glProgramUniform3uiv(program, location, count, value);
}
inline void glProgramUniform3uivEXT(GLuint program, GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glProgramUniform3uivEXT(program, location, count, value);
}
inline void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3) noexcept
{
        opengl_functions::glProgramUniform4d(program, location, v0, v1, v2, v3);
}
inline void glProgramUniform4dEXT(GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) noexcept
{
        opengl_functions::glProgramUniform4dEXT(program, location, x, y, z, w);
}
inline void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniform4dv(program, location, count, value);
}
inline void glProgramUniform4dvEXT(GLuint program, GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniform4dvEXT(program, location, count, value);
}
inline void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) noexcept
{
        opengl_functions::glProgramUniform4f(program, location, v0, v1, v2, v3);
}
inline void glProgramUniform4fEXT(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) noexcept
{
        opengl_functions::glProgramUniform4fEXT(program, location, v0, v1, v2, v3);
}
inline void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniform4fv(program, location, count, value);
}
inline void glProgramUniform4fvEXT(GLuint program, GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniform4fvEXT(program, location, count, value);
}
inline void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) noexcept
{
        opengl_functions::glProgramUniform4i(program, location, v0, v1, v2, v3);
}
inline void glProgramUniform4i64ARB(GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w) noexcept
{
        opengl_functions::glProgramUniform4i64ARB(program, location, x, y, z, w);
}
inline void glProgramUniform4i64NV(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w) noexcept
{
        opengl_functions::glProgramUniform4i64NV(program, location, x, y, z, w);
}
inline void glProgramUniform4i64vARB(GLuint program, GLint location, GLsizei count, const GLint64 *value) noexcept
{
        opengl_functions::glProgramUniform4i64vARB(program, location, count, value);
}
inline void glProgramUniform4i64vNV(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) noexcept
{
        opengl_functions::glProgramUniform4i64vNV(program, location, count, value);
}
inline void glProgramUniform4iEXT(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) noexcept
{
        opengl_functions::glProgramUniform4iEXT(program, location, v0, v1, v2, v3);
}
inline void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glProgramUniform4iv(program, location, count, value);
}
inline void glProgramUniform4ivEXT(GLuint program, GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glProgramUniform4ivEXT(program, location, count, value);
}
inline void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) noexcept
{
        opengl_functions::glProgramUniform4ui(program, location, v0, v1, v2, v3);
}
inline void glProgramUniform4ui64ARB(GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w) noexcept
{
        opengl_functions::glProgramUniform4ui64ARB(program, location, x, y, z, w);
}
inline void glProgramUniform4ui64NV(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w) noexcept
{
        opengl_functions::glProgramUniform4ui64NV(program, location, x, y, z, w);
}
inline void glProgramUniform4ui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glProgramUniform4ui64vARB(program, location, count, value);
}
inline void glProgramUniform4ui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glProgramUniform4ui64vNV(program, location, count, value);
}
inline void glProgramUniform4uiEXT(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) noexcept
{
        opengl_functions::glProgramUniform4uiEXT(program, location, v0, v1, v2, v3);
}
inline void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glProgramUniform4uiv(program, location, count, value);
}
inline void glProgramUniform4uivEXT(GLuint program, GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glProgramUniform4uivEXT(program, location, count, value);
}
inline void glProgramUniformHandleui64ARB(GLuint program, GLint location, GLuint64 value) noexcept
{
        opengl_functions::glProgramUniformHandleui64ARB(program, location, value);
}
inline void glProgramUniformHandleui64NV(GLuint program, GLint location, GLuint64 value) noexcept
{
        opengl_functions::glProgramUniformHandleui64NV(program, location, value);
}
inline void glProgramUniformHandleui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64 *values) noexcept
{
        opengl_functions::glProgramUniformHandleui64vARB(program, location, count, values);
}
inline void glProgramUniformHandleui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64 *values) noexcept
{
        opengl_functions::glProgramUniformHandleui64vNV(program, location, count, values);
}
inline void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2dv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2dvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2dvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2fv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2fvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2x3dv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2x3dvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2x3dvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2x3fv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2x3fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2x3fvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2x4dv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2x4dvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2x4dvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2x4fv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix2x4fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix2x4fvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3dv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3dvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3dvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3fv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3fvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3x2dv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3x2dvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3x2dvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3x2fv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3x2fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3x2fvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3x4dv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3x4dvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3x4dvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3x4fv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix3x4fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix3x4fvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4dv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4dvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4dvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4fv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4fvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4x2dv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4x2dvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4x2dvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4x2fv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4x2fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4x2fvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4x3dv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4x3dvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4x3dvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4x3fv(program, location, count, transpose, value);
}
inline void glProgramUniformMatrix4x3fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glProgramUniformMatrix4x3fvEXT(program, location, count, transpose, value);
}
inline void glProgramUniformui64NV(GLuint program, GLint location, GLuint64EXT value) noexcept
{
        opengl_functions::glProgramUniformui64NV(program, location, value);
}
inline void glProgramUniformui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glProgramUniformui64vNV(program, location, count, value);
}
inline void glProvokingVertex(GLenum mode) noexcept
{
        opengl_functions::glProvokingVertex(mode);
}
inline void glPushClientAttribDefaultEXT(GLbitfield mask) noexcept
{
        opengl_functions::glPushClientAttribDefaultEXT(mask);
}
inline void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar *message) noexcept
{
        opengl_functions::glPushDebugGroup(source, id, length, message);
}
inline void glPushGroupMarkerEXT(GLsizei length, const GLchar *marker) noexcept
{
        opengl_functions::glPushGroupMarkerEXT(length, marker);
}
inline void glQueryCounter(GLuint id, GLenum target) noexcept
{
        opengl_functions::glQueryCounter(id, target);
}
inline void glRasterSamplesEXT(GLuint samples, GLboolean fixedsamplelocations) noexcept
{
        opengl_functions::glRasterSamplesEXT(samples, fixedsamplelocations);
}
inline void glReadBuffer(GLenum src) noexcept
{
        opengl_functions::glReadBuffer(src);
}
inline void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) noexcept
{
        opengl_functions::glReadPixels(x, y, width, height, format, type, pixels);
}
inline void glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data) noexcept
{
        opengl_functions::glReadnPixels(x, y, width, height, format, type, bufSize, data);
}
inline void glReadnPixelsARB(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data) noexcept
{
        opengl_functions::glReadnPixelsARB(x, y, width, height, format, type, bufSize, data);
}
inline void glReleaseShaderCompiler() noexcept
{
        opengl_functions::glReleaseShaderCompiler();
}
inline void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glRenderbufferStorage(target, internalformat, width, height);
}
inline void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}
inline void glRenderbufferStorageMultisampleAdvancedAMD(GLenum target, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glRenderbufferStorageMultisampleAdvancedAMD(target, samples, storageSamples, internalformat, width, height);
}
inline void glRenderbufferStorageMultisampleCoverageNV(GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glRenderbufferStorageMultisampleCoverageNV(target, coverageSamples, colorSamples, internalformat, width, height);
}
inline void glResetMemoryObjectParameterNV(GLuint memory, GLenum pname) noexcept
{
        opengl_functions::glResetMemoryObjectParameterNV(memory, pname);
}
inline void glResolveDepthValuesNV() noexcept
{
        opengl_functions::glResolveDepthValuesNV();
}
inline void glResumeTransformFeedback() noexcept
{
        opengl_functions::glResumeTransformFeedback();
}
inline void glSampleCoverage(GLfloat value, GLboolean invert) noexcept
{
        opengl_functions::glSampleCoverage(value, invert);
}
inline void glSampleMaski(GLuint maskNumber, GLbitfield mask) noexcept
{
        opengl_functions::glSampleMaski(maskNumber, mask);
}
inline void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *param) noexcept
{
        opengl_functions::glSamplerParameterIiv(sampler, pname, param);
}
inline void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *param) noexcept
{
        opengl_functions::glSamplerParameterIuiv(sampler, pname, param);
}
inline void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) noexcept
{
        opengl_functions::glSamplerParameterf(sampler, pname, param);
}
inline void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param) noexcept
{
        opengl_functions::glSamplerParameterfv(sampler, pname, param);
}
inline void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) noexcept
{
        opengl_functions::glSamplerParameteri(sampler, pname, param);
}
inline void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param) noexcept
{
        opengl_functions::glSamplerParameteriv(sampler, pname, param);
}
inline void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glScissor(x, y, width, height);
}
inline void glScissorArrayv(GLuint first, GLsizei count, const GLint *v) noexcept
{
        opengl_functions::glScissorArrayv(first, count, v);
}
inline void glScissorExclusiveArrayvNV(GLuint first, GLsizei count, const GLint *v) noexcept
{
        opengl_functions::glScissorExclusiveArrayvNV(first, count, v);
}
inline void glScissorExclusiveNV(GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glScissorExclusiveNV(x, y, width, height);
}
inline void glScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glScissorIndexed(index, left, bottom, width, height);
}
inline void glScissorIndexedv(GLuint index, const GLint *v) noexcept
{
        opengl_functions::glScissorIndexedv(index, v);
}
inline void glSecondaryColorFormatNV(GLint size, GLenum type, GLsizei stride) noexcept
{
        opengl_functions::glSecondaryColorFormatNV(size, type, stride);
}
inline void glSelectPerfMonitorCountersAMD(GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList) noexcept
{
        opengl_functions::glSelectPerfMonitorCountersAMD(monitor, enable, group, numCounters, counterList);
}
inline void glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length) noexcept
{
        opengl_functions::glShaderBinary(count, shaders, binaryformat, binary, length);
}
inline void glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) noexcept
{
        opengl_functions::glShaderSource(shader, count, string, length);
}
inline void glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) noexcept
{
        opengl_functions::glShaderStorageBlockBinding(program, storageBlockIndex, storageBlockBinding);
}
inline void glShadingRateImageBarrierNV(GLboolean synchronize) noexcept
{
        opengl_functions::glShadingRateImageBarrierNV(synchronize);
}
inline void glShadingRateImagePaletteNV(GLuint viewport, GLuint first, GLsizei count, const GLenum *rates) noexcept
{
        opengl_functions::glShadingRateImagePaletteNV(viewport, first, count, rates);
}
inline void glShadingRateSampleOrderCustomNV(GLenum rate, GLuint samples, const GLint *locations) noexcept
{
        opengl_functions::glShadingRateSampleOrderCustomNV(rate, samples, locations);
}
inline void glShadingRateSampleOrderNV(GLenum order) noexcept
{
        opengl_functions::glShadingRateSampleOrderNV(order);
}
inline void glSignalVkFenceNV(GLuint64 vkFence) noexcept
{
        opengl_functions::glSignalVkFenceNV(vkFence);
}
inline void glSignalVkSemaphoreNV(GLuint64 vkSemaphore) noexcept
{
        opengl_functions::glSignalVkSemaphoreNV(vkSemaphore);
}
inline void glSpecializeShader(GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue) noexcept
{
        opengl_functions::glSpecializeShader(shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue);
}
inline void glSpecializeShaderARB(GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue) noexcept
{
        opengl_functions::glSpecializeShaderARB(shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue);
}
inline void glStateCaptureNV(GLuint state, GLenum mode) noexcept
{
        opengl_functions::glStateCaptureNV(state, mode);
}
inline void glStencilFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues) noexcept
{
        opengl_functions::glStencilFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues);
}
inline void glStencilFillPathNV(GLuint path, GLenum fillMode, GLuint mask) noexcept
{
        opengl_functions::glStencilFillPathNV(path, fillMode, mask);
}
inline void glStencilFunc(GLenum func, GLint ref, GLuint mask) noexcept
{
        opengl_functions::glStencilFunc(func, ref, mask);
}
inline void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) noexcept
{
        opengl_functions::glStencilFuncSeparate(face, func, ref, mask);
}
inline void glStencilMask(GLuint mask) noexcept
{
        opengl_functions::glStencilMask(mask);
}
inline void glStencilMaskSeparate(GLenum face, GLuint mask) noexcept
{
        opengl_functions::glStencilMaskSeparate(face, mask);
}
inline void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) noexcept
{
        opengl_functions::glStencilOp(fail, zfail, zpass);
}
inline void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) noexcept
{
        opengl_functions::glStencilOpSeparate(face, sfail, dpfail, dppass);
}
inline void glStencilStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues) noexcept
{
        opengl_functions::glStencilStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues);
}
inline void glStencilStrokePathNV(GLuint path, GLint reference, GLuint mask) noexcept
{
        opengl_functions::glStencilStrokePathNV(path, reference, mask);
}
inline void glStencilThenCoverFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues) noexcept
{
        opengl_functions::glStencilThenCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues);
}
inline void glStencilThenCoverFillPathNV(GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode) noexcept
{
        opengl_functions::glStencilThenCoverFillPathNV(path, fillMode, mask, coverMode);
}
inline void glStencilThenCoverStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues) noexcept
{
        opengl_functions::glStencilThenCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues);
}
inline void glStencilThenCoverStrokePathNV(GLuint path, GLint reference, GLuint mask, GLenum coverMode) noexcept
{
        opengl_functions::glStencilThenCoverStrokePathNV(path, reference, mask, coverMode);
}
inline void glSubpixelPrecisionBiasNV(GLuint xbits, GLuint ybits) noexcept
{
        opengl_functions::glSubpixelPrecisionBiasNV(xbits, ybits);
}
inline void glTexAttachMemoryNV(GLenum target, GLuint memory, GLuint64 offset) noexcept
{
        opengl_functions::glTexAttachMemoryNV(target, memory, offset);
}
inline void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) noexcept
{
        opengl_functions::glTexBuffer(target, internalformat, buffer);
}
inline void glTexBufferARB(GLenum target, GLenum internalformat, GLuint buffer) noexcept
{
        opengl_functions::glTexBufferARB(target, internalformat, buffer);
}
inline void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) noexcept
{
        opengl_functions::glTexBufferRange(target, internalformat, buffer, offset, size);
}
inline void glTexCoordFormatNV(GLint size, GLenum type, GLsizei stride) noexcept
{
        opengl_functions::glTexCoordFormatNV(size, type, stride);
}
inline void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}
inline void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}
inline void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) noexcept
{
        opengl_functions::glTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}
inline void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}
inline void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) noexcept
{
        opengl_functions::glTexImage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}
inline void glTexPageCommitmentARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit) noexcept
{
        opengl_functions::glTexPageCommitmentARB(target, level, xoffset, yoffset, zoffset, width, height, depth, commit);
}
inline void glTexParameterIiv(GLenum target, GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glTexParameterIiv(target, pname, params);
}
inline void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint *params) noexcept
{
        opengl_functions::glTexParameterIuiv(target, pname, params);
}
inline void glTexParameterf(GLenum target, GLenum pname, GLfloat param) noexcept
{
        opengl_functions::glTexParameterf(target, pname, param);
}
inline void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) noexcept
{
        opengl_functions::glTexParameterfv(target, pname, params);
}
inline void glTexParameteri(GLenum target, GLenum pname, GLint param) noexcept
{
        opengl_functions::glTexParameteri(target, pname, param);
}
inline void glTexParameteriv(GLenum target, GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glTexParameteriv(target, pname, params);
}
inline void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) noexcept
{
        opengl_functions::glTexStorage1D(target, levels, internalformat, width);
}
inline void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glTexStorage2D(target, levels, internalformat, width, height);
}
inline void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) noexcept
{
        opengl_functions::glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}
inline void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) noexcept
{
        opengl_functions::glTexStorage3D(target, levels, internalformat, width, height, depth);
}
inline void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) noexcept
{
        opengl_functions::glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}
inline void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}
inline void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}
inline void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
inline void glTextureAttachMemoryNV(GLuint texture, GLuint memory, GLuint64 offset) noexcept
{
        opengl_functions::glTextureAttachMemoryNV(texture, memory, offset);
}
inline void glTextureBarrier() noexcept
{
        opengl_functions::glTextureBarrier();
}
inline void glTextureBarrierNV() noexcept
{
        opengl_functions::glTextureBarrierNV();
}
inline void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer) noexcept
{
        opengl_functions::glTextureBuffer(texture, internalformat, buffer);
}
inline void glTextureBufferEXT(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) noexcept
{
        opengl_functions::glTextureBufferEXT(texture, target, internalformat, buffer);
}
inline void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) noexcept
{
        opengl_functions::glTextureBufferRange(texture, internalformat, buffer, offset, size);
}
inline void glTextureBufferRangeEXT(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) noexcept
{
        opengl_functions::glTextureBufferRangeEXT(texture, target, internalformat, buffer, offset, size);
}
inline void glTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTextureImage1DEXT(texture, target, level, internalformat, width, border, format, type, pixels);
}
inline void glTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTextureImage2DEXT(texture, target, level, internalformat, width, height, border, format, type, pixels);
}
inline void glTextureImage3DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTextureImage3DEXT(texture, target, level, internalformat, width, height, depth, border, format, type, pixels);
}
inline void glTexturePageCommitmentEXT(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit) noexcept
{
        opengl_functions::glTexturePageCommitmentEXT(texture, level, xoffset, yoffset, zoffset, width, height, depth, commit);
}
inline void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glTextureParameterIiv(texture, pname, params);
}
inline void glTextureParameterIivEXT(GLuint texture, GLenum target, GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glTextureParameterIivEXT(texture, target, pname, params);
}
inline void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint *params) noexcept
{
        opengl_functions::glTextureParameterIuiv(texture, pname, params);
}
inline void glTextureParameterIuivEXT(GLuint texture, GLenum target, GLenum pname, const GLuint *params) noexcept
{
        opengl_functions::glTextureParameterIuivEXT(texture, target, pname, params);
}
inline void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) noexcept
{
        opengl_functions::glTextureParameterf(texture, pname, param);
}
inline void glTextureParameterfEXT(GLuint texture, GLenum target, GLenum pname, GLfloat param) noexcept
{
        opengl_functions::glTextureParameterfEXT(texture, target, pname, param);
}
inline void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat *param) noexcept
{
        opengl_functions::glTextureParameterfv(texture, pname, param);
}
inline void glTextureParameterfvEXT(GLuint texture, GLenum target, GLenum pname, const GLfloat *params) noexcept
{
        opengl_functions::glTextureParameterfvEXT(texture, target, pname, params);
}
inline void glTextureParameteri(GLuint texture, GLenum pname, GLint param) noexcept
{
        opengl_functions::glTextureParameteri(texture, pname, param);
}
inline void glTextureParameteriEXT(GLuint texture, GLenum target, GLenum pname, GLint param) noexcept
{
        opengl_functions::glTextureParameteriEXT(texture, target, pname, param);
}
inline void glTextureParameteriv(GLuint texture, GLenum pname, const GLint *param) noexcept
{
        opengl_functions::glTextureParameteriv(texture, pname, param);
}
inline void glTextureParameterivEXT(GLuint texture, GLenum target, GLenum pname, const GLint *params) noexcept
{
        opengl_functions::glTextureParameterivEXT(texture, target, pname, params);
}
inline void glTextureRenderbufferEXT(GLuint texture, GLenum target, GLuint renderbuffer) noexcept
{
        opengl_functions::glTextureRenderbufferEXT(texture, target, renderbuffer);
}
inline void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) noexcept
{
        opengl_functions::glTextureStorage1D(texture, levels, internalformat, width);
}
inline void glTextureStorage1DEXT(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) noexcept
{
        opengl_functions::glTextureStorage1DEXT(texture, target, levels, internalformat, width);
}
inline void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glTextureStorage2D(texture, levels, internalformat, width, height);
}
inline void glTextureStorage2DEXT(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glTextureStorage2DEXT(texture, target, levels, internalformat, width, height);
}
inline void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) noexcept
{
        opengl_functions::glTextureStorage2DMultisample(texture, samples, internalformat, width, height, fixedsamplelocations);
}
inline void glTextureStorage2DMultisampleEXT(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) noexcept
{
        opengl_functions::glTextureStorage2DMultisampleEXT(texture, target, samples, internalformat, width, height, fixedsamplelocations);
}
inline void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) noexcept
{
        opengl_functions::glTextureStorage3D(texture, levels, internalformat, width, height, depth);
}
inline void glTextureStorage3DEXT(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) noexcept
{
        opengl_functions::glTextureStorage3DEXT(texture, target, levels, internalformat, width, height, depth);
}
inline void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) noexcept
{
        opengl_functions::glTextureStorage3DMultisample(texture, samples, internalformat, width, height, depth, fixedsamplelocations);
}
inline void glTextureStorage3DMultisampleEXT(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) noexcept
{
        opengl_functions::glTextureStorage3DMultisampleEXT(texture, target, samples, internalformat, width, height, depth, fixedsamplelocations);
}
inline void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTextureSubImage1D(texture, level, xoffset, width, format, type, pixels);
}
inline void glTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTextureSubImage1DEXT(texture, target, level, xoffset, width, format, type, pixels);
}
inline void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, pixels);
}
inline void glTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, type, pixels);
}
inline void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
inline void glTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) noexcept
{
        opengl_functions::glTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
inline void glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) noexcept
{
        opengl_functions::glTextureView(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers);
}
inline void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer) noexcept
{
        opengl_functions::glTransformFeedbackBufferBase(xfb, index, buffer);
}
inline void glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) noexcept
{
        opengl_functions::glTransformFeedbackBufferRange(xfb, index, buffer, offset, size);
}
inline void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode) noexcept
{
        opengl_functions::glTransformFeedbackVaryings(program, count, varyings, bufferMode);
}
inline void glTransformPathNV(GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues) noexcept
{
        opengl_functions::glTransformPathNV(resultPath, srcPath, transformType, transformValues);
}
inline void glUniform1d(GLint location, GLdouble x) noexcept
{
        opengl_functions::glUniform1d(location, x);
}
inline void glUniform1dv(GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glUniform1dv(location, count, value);
}
inline void glUniform1f(GLint location, GLfloat v0) noexcept
{
        opengl_functions::glUniform1f(location, v0);
}
inline void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glUniform1fv(location, count, value);
}
inline void glUniform1i(GLint location, GLint v0) noexcept
{
        opengl_functions::glUniform1i(location, v0);
}
inline void glUniform1i64ARB(GLint location, GLint64 x) noexcept
{
        opengl_functions::glUniform1i64ARB(location, x);
}
inline void glUniform1i64NV(GLint location, GLint64EXT x) noexcept
{
        opengl_functions::glUniform1i64NV(location, x);
}
inline void glUniform1i64vARB(GLint location, GLsizei count, const GLint64 *value) noexcept
{
        opengl_functions::glUniform1i64vARB(location, count, value);
}
inline void glUniform1i64vNV(GLint location, GLsizei count, const GLint64EXT *value) noexcept
{
        opengl_functions::glUniform1i64vNV(location, count, value);
}
inline void glUniform1iv(GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glUniform1iv(location, count, value);
}
inline void glUniform1ui(GLint location, GLuint v0) noexcept
{
        opengl_functions::glUniform1ui(location, v0);
}
inline void glUniform1ui64ARB(GLint location, GLuint64 x) noexcept
{
        opengl_functions::glUniform1ui64ARB(location, x);
}
inline void glUniform1ui64NV(GLint location, GLuint64EXT x) noexcept
{
        opengl_functions::glUniform1ui64NV(location, x);
}
inline void glUniform1ui64vARB(GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glUniform1ui64vARB(location, count, value);
}
inline void glUniform1ui64vNV(GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glUniform1ui64vNV(location, count, value);
}
inline void glUniform1uiv(GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glUniform1uiv(location, count, value);
}
inline void glUniform2d(GLint location, GLdouble x, GLdouble y) noexcept
{
        opengl_functions::glUniform2d(location, x, y);
}
inline void glUniform2dv(GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glUniform2dv(location, count, value);
}
inline void glUniform2f(GLint location, GLfloat v0, GLfloat v1) noexcept
{
        opengl_functions::glUniform2f(location, v0, v1);
}
inline void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glUniform2fv(location, count, value);
}
inline void glUniform2i(GLint location, GLint v0, GLint v1) noexcept
{
        opengl_functions::glUniform2i(location, v0, v1);
}
inline void glUniform2i64ARB(GLint location, GLint64 x, GLint64 y) noexcept
{
        opengl_functions::glUniform2i64ARB(location, x, y);
}
inline void glUniform2i64NV(GLint location, GLint64EXT x, GLint64EXT y) noexcept
{
        opengl_functions::glUniform2i64NV(location, x, y);
}
inline void glUniform2i64vARB(GLint location, GLsizei count, const GLint64 *value) noexcept
{
        opengl_functions::glUniform2i64vARB(location, count, value);
}
inline void glUniform2i64vNV(GLint location, GLsizei count, const GLint64EXT *value) noexcept
{
        opengl_functions::glUniform2i64vNV(location, count, value);
}
inline void glUniform2iv(GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glUniform2iv(location, count, value);
}
inline void glUniform2ui(GLint location, GLuint v0, GLuint v1) noexcept
{
        opengl_functions::glUniform2ui(location, v0, v1);
}
inline void glUniform2ui64ARB(GLint location, GLuint64 x, GLuint64 y) noexcept
{
        opengl_functions::glUniform2ui64ARB(location, x, y);
}
inline void glUniform2ui64NV(GLint location, GLuint64EXT x, GLuint64EXT y) noexcept
{
        opengl_functions::glUniform2ui64NV(location, x, y);
}
inline void glUniform2ui64vARB(GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glUniform2ui64vARB(location, count, value);
}
inline void glUniform2ui64vNV(GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glUniform2ui64vNV(location, count, value);
}
inline void glUniform2uiv(GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glUniform2uiv(location, count, value);
}
inline void glUniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z) noexcept
{
        opengl_functions::glUniform3d(location, x, y, z);
}
inline void glUniform3dv(GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glUniform3dv(location, count, value);
}
inline void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) noexcept
{
        opengl_functions::glUniform3f(location, v0, v1, v2);
}
inline void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glUniform3fv(location, count, value);
}
inline void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) noexcept
{
        opengl_functions::glUniform3i(location, v0, v1, v2);
}
inline void glUniform3i64ARB(GLint location, GLint64 x, GLint64 y, GLint64 z) noexcept
{
        opengl_functions::glUniform3i64ARB(location, x, y, z);
}
inline void glUniform3i64NV(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z) noexcept
{
        opengl_functions::glUniform3i64NV(location, x, y, z);
}
inline void glUniform3i64vARB(GLint location, GLsizei count, const GLint64 *value) noexcept
{
        opengl_functions::glUniform3i64vARB(location, count, value);
}
inline void glUniform3i64vNV(GLint location, GLsizei count, const GLint64EXT *value) noexcept
{
        opengl_functions::glUniform3i64vNV(location, count, value);
}
inline void glUniform3iv(GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glUniform3iv(location, count, value);
}
inline void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) noexcept
{
        opengl_functions::glUniform3ui(location, v0, v1, v2);
}
inline void glUniform3ui64ARB(GLint location, GLuint64 x, GLuint64 y, GLuint64 z) noexcept
{
        opengl_functions::glUniform3ui64ARB(location, x, y, z);
}
inline void glUniform3ui64NV(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z) noexcept
{
        opengl_functions::glUniform3ui64NV(location, x, y, z);
}
inline void glUniform3ui64vARB(GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glUniform3ui64vARB(location, count, value);
}
inline void glUniform3ui64vNV(GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glUniform3ui64vNV(location, count, value);
}
inline void glUniform3uiv(GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glUniform3uiv(location, count, value);
}
inline void glUniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) noexcept
{
        opengl_functions::glUniform4d(location, x, y, z, w);
}
inline void glUniform4dv(GLint location, GLsizei count, const GLdouble *value) noexcept
{
        opengl_functions::glUniform4dv(location, count, value);
}
inline void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) noexcept
{
        opengl_functions::glUniform4f(location, v0, v1, v2, v3);
}
inline void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) noexcept
{
        opengl_functions::glUniform4fv(location, count, value);
}
inline void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) noexcept
{
        opengl_functions::glUniform4i(location, v0, v1, v2, v3);
}
inline void glUniform4i64ARB(GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w) noexcept
{
        opengl_functions::glUniform4i64ARB(location, x, y, z, w);
}
inline void glUniform4i64NV(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w) noexcept
{
        opengl_functions::glUniform4i64NV(location, x, y, z, w);
}
inline void glUniform4i64vARB(GLint location, GLsizei count, const GLint64 *value) noexcept
{
        opengl_functions::glUniform4i64vARB(location, count, value);
}
inline void glUniform4i64vNV(GLint location, GLsizei count, const GLint64EXT *value) noexcept
{
        opengl_functions::glUniform4i64vNV(location, count, value);
}
inline void glUniform4iv(GLint location, GLsizei count, const GLint *value) noexcept
{
        opengl_functions::glUniform4iv(location, count, value);
}
inline void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) noexcept
{
        opengl_functions::glUniform4ui(location, v0, v1, v2, v3);
}
inline void glUniform4ui64ARB(GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w) noexcept
{
        opengl_functions::glUniform4ui64ARB(location, x, y, z, w);
}
inline void glUniform4ui64NV(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w) noexcept
{
        opengl_functions::glUniform4ui64NV(location, x, y, z, w);
}
inline void glUniform4ui64vARB(GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glUniform4ui64vARB(location, count, value);
}
inline void glUniform4ui64vNV(GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glUniform4ui64vNV(location, count, value);
}
inline void glUniform4uiv(GLint location, GLsizei count, const GLuint *value) noexcept
{
        opengl_functions::glUniform4uiv(location, count, value);
}
inline void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) noexcept
{
        opengl_functions::glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
}
inline void glUniformHandleui64ARB(GLint location, GLuint64 value) noexcept
{
        opengl_functions::glUniformHandleui64ARB(location, value);
}
inline void glUniformHandleui64NV(GLint location, GLuint64 value) noexcept
{
        opengl_functions::glUniformHandleui64NV(location, value);
}
inline void glUniformHandleui64vARB(GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glUniformHandleui64vARB(location, count, value);
}
inline void glUniformHandleui64vNV(GLint location, GLsizei count, const GLuint64 *value) noexcept
{
        opengl_functions::glUniformHandleui64vNV(location, count, value);
}
inline void glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glUniformMatrix2dv(location, count, transpose, value);
}
inline void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glUniformMatrix2fv(location, count, transpose, value);
}
inline void glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glUniformMatrix2x3dv(location, count, transpose, value);
}
inline void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glUniformMatrix2x3fv(location, count, transpose, value);
}
inline void glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glUniformMatrix2x4dv(location, count, transpose, value);
}
inline void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glUniformMatrix2x4fv(location, count, transpose, value);
}
inline void glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glUniformMatrix3dv(location, count, transpose, value);
}
inline void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glUniformMatrix3fv(location, count, transpose, value);
}
inline void glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glUniformMatrix3x2dv(location, count, transpose, value);
}
inline void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glUniformMatrix3x2fv(location, count, transpose, value);
}
inline void glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glUniformMatrix3x4dv(location, count, transpose, value);
}
inline void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glUniformMatrix3x4fv(location, count, transpose, value);
}
inline void glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glUniformMatrix4dv(location, count, transpose, value);
}
inline void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glUniformMatrix4fv(location, count, transpose, value);
}
inline void glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glUniformMatrix4x2dv(location, count, transpose, value);
}
inline void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glUniformMatrix4x2fv(location, count, transpose, value);
}
inline void glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) noexcept
{
        opengl_functions::glUniformMatrix4x3dv(location, count, transpose, value);
}
inline void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) noexcept
{
        opengl_functions::glUniformMatrix4x3fv(location, count, transpose, value);
}
inline void glUniformSubroutinesuiv(GLenum shadertype, GLsizei count, const GLuint *indices) noexcept
{
        opengl_functions::glUniformSubroutinesuiv(shadertype, count, indices);
}
inline void glUniformui64NV(GLint location, GLuint64EXT value) noexcept
{
        opengl_functions::glUniformui64NV(location, value);
}
inline void glUniformui64vNV(GLint location, GLsizei count, const GLuint64EXT *value) noexcept
{
        opengl_functions::glUniformui64vNV(location, count, value);
}
inline GLboolean glUnmapBuffer(GLenum target) noexcept
{
        return opengl_functions::glUnmapBuffer(target);
}
inline GLboolean glUnmapNamedBuffer(GLuint buffer) noexcept
{
        return opengl_functions::glUnmapNamedBuffer(buffer);
}
inline GLboolean glUnmapNamedBufferEXT(GLuint buffer) noexcept
{
        return opengl_functions::glUnmapNamedBufferEXT(buffer);
}
inline void glUseProgram(GLuint program) noexcept
{
        opengl_functions::glUseProgram(program);
}
inline void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) noexcept
{
        opengl_functions::glUseProgramStages(pipeline, stages, program);
}
inline void glUseShaderProgramEXT(GLenum type, GLuint program) noexcept
{
        opengl_functions::glUseShaderProgramEXT(type, program);
}
inline void glValidateProgram(GLuint program) noexcept
{
        opengl_functions::glValidateProgram(program);
}
inline void glValidateProgramPipeline(GLuint pipeline) noexcept
{
        opengl_functions::glValidateProgramPipeline(pipeline);
}
inline void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex) noexcept
{
        opengl_functions::glVertexArrayAttribBinding(vaobj, attribindex, bindingindex);
}
inline void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) noexcept
{
        opengl_functions::glVertexArrayAttribFormat(vaobj, attribindex, size, type, normalized, relativeoffset);
}
inline void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) noexcept
{
        opengl_functions::glVertexArrayAttribIFormat(vaobj, attribindex, size, type, relativeoffset);
}
inline void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) noexcept
{
        opengl_functions::glVertexArrayAttribLFormat(vaobj, attribindex, size, type, relativeoffset);
}
inline void glVertexArrayBindVertexBufferEXT(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) noexcept
{
        opengl_functions::glVertexArrayBindVertexBufferEXT(vaobj, bindingindex, buffer, offset, stride);
}
inline void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor) noexcept
{
        opengl_functions::glVertexArrayBindingDivisor(vaobj, bindingindex, divisor);
}
inline void glVertexArrayColorOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayColorOffsetEXT(vaobj, buffer, size, type, stride, offset);
}
inline void glVertexArrayEdgeFlagOffsetEXT(GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayEdgeFlagOffsetEXT(vaobj, buffer, stride, offset);
}
inline void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer) noexcept
{
        opengl_functions::glVertexArrayElementBuffer(vaobj, buffer);
}
inline void glVertexArrayFogCoordOffsetEXT(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayFogCoordOffsetEXT(vaobj, buffer, type, stride, offset);
}
inline void glVertexArrayIndexOffsetEXT(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayIndexOffsetEXT(vaobj, buffer, type, stride, offset);
}
inline void glVertexArrayMultiTexCoordOffsetEXT(GLuint vaobj, GLuint buffer, GLenum texunit, GLint size, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayMultiTexCoordOffsetEXT(vaobj, buffer, texunit, size, type, stride, offset);
}
inline void glVertexArrayNormalOffsetEXT(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayNormalOffsetEXT(vaobj, buffer, type, stride, offset);
}
inline void glVertexArraySecondaryColorOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArraySecondaryColorOffsetEXT(vaobj, buffer, size, type, stride, offset);
}
inline void glVertexArrayTexCoordOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayTexCoordOffsetEXT(vaobj, buffer, size, type, stride, offset);
}
inline void glVertexArrayVertexAttribBindingEXT(GLuint vaobj, GLuint attribindex, GLuint bindingindex) noexcept
{
        opengl_functions::glVertexArrayVertexAttribBindingEXT(vaobj, attribindex, bindingindex);
}
inline void glVertexArrayVertexAttribDivisorEXT(GLuint vaobj, GLuint index, GLuint divisor) noexcept
{
        opengl_functions::glVertexArrayVertexAttribDivisorEXT(vaobj, index, divisor);
}
inline void glVertexArrayVertexAttribFormatEXT(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) noexcept
{
        opengl_functions::glVertexArrayVertexAttribFormatEXT(vaobj, attribindex, size, type, normalized, relativeoffset);
}
inline void glVertexArrayVertexAttribIFormatEXT(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) noexcept
{
        opengl_functions::glVertexArrayVertexAttribIFormatEXT(vaobj, attribindex, size, type, relativeoffset);
}
inline void glVertexArrayVertexAttribIOffsetEXT(GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayVertexAttribIOffsetEXT(vaobj, buffer, index, size, type, stride, offset);
}
inline void glVertexArrayVertexAttribLFormatEXT(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) noexcept
{
        opengl_functions::glVertexArrayVertexAttribLFormatEXT(vaobj, attribindex, size, type, relativeoffset);
}
inline void glVertexArrayVertexAttribLOffsetEXT(GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayVertexAttribLOffsetEXT(vaobj, buffer, index, size, type, stride, offset);
}
inline void glVertexArrayVertexAttribOffsetEXT(GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayVertexAttribOffsetEXT(vaobj, buffer, index, size, type, normalized, stride, offset);
}
inline void glVertexArrayVertexBindingDivisorEXT(GLuint vaobj, GLuint bindingindex, GLuint divisor) noexcept
{
        opengl_functions::glVertexArrayVertexBindingDivisorEXT(vaobj, bindingindex, divisor);
}
inline void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) noexcept
{
        opengl_functions::glVertexArrayVertexBuffer(vaobj, bindingindex, buffer, offset, stride);
}
inline void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides) noexcept
{
        opengl_functions::glVertexArrayVertexBuffers(vaobj, first, count, buffers, offsets, strides);
}
inline void glVertexArrayVertexOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset) noexcept
{
        opengl_functions::glVertexArrayVertexOffsetEXT(vaobj, buffer, size, type, stride, offset);
}
inline void glVertexAttrib1d(GLuint index, GLdouble x) noexcept
{
        opengl_functions::glVertexAttrib1d(index, x);
}
inline void glVertexAttrib1dv(GLuint index, const GLdouble *v) noexcept
{
        opengl_functions::glVertexAttrib1dv(index, v);
}
inline void glVertexAttrib1f(GLuint index, GLfloat x) noexcept
{
        opengl_functions::glVertexAttrib1f(index, x);
}
inline void glVertexAttrib1fv(GLuint index, const GLfloat *v) noexcept
{
        opengl_functions::glVertexAttrib1fv(index, v);
}
inline void glVertexAttrib1s(GLuint index, GLshort x) noexcept
{
        opengl_functions::glVertexAttrib1s(index, x);
}
inline void glVertexAttrib1sv(GLuint index, const GLshort *v) noexcept
{
        opengl_functions::glVertexAttrib1sv(index, v);
}
inline void glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y) noexcept
{
        opengl_functions::glVertexAttrib2d(index, x, y);
}
inline void glVertexAttrib2dv(GLuint index, const GLdouble *v) noexcept
{
        opengl_functions::glVertexAttrib2dv(index, v);
}
inline void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) noexcept
{
        opengl_functions::glVertexAttrib2f(index, x, y);
}
inline void glVertexAttrib2fv(GLuint index, const GLfloat *v) noexcept
{
        opengl_functions::glVertexAttrib2fv(index, v);
}
inline void glVertexAttrib2s(GLuint index, GLshort x, GLshort y) noexcept
{
        opengl_functions::glVertexAttrib2s(index, x, y);
}
inline void glVertexAttrib2sv(GLuint index, const GLshort *v) noexcept
{
        opengl_functions::glVertexAttrib2sv(index, v);
}
inline void glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) noexcept
{
        opengl_functions::glVertexAttrib3d(index, x, y, z);
}
inline void glVertexAttrib3dv(GLuint index, const GLdouble *v) noexcept
{
        opengl_functions::glVertexAttrib3dv(index, v);
}
inline void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) noexcept
{
        opengl_functions::glVertexAttrib3f(index, x, y, z);
}
inline void glVertexAttrib3fv(GLuint index, const GLfloat *v) noexcept
{
        opengl_functions::glVertexAttrib3fv(index, v);
}
inline void glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z) noexcept
{
        opengl_functions::glVertexAttrib3s(index, x, y, z);
}
inline void glVertexAttrib3sv(GLuint index, const GLshort *v) noexcept
{
        opengl_functions::glVertexAttrib3sv(index, v);
}
inline void glVertexAttrib4Nbv(GLuint index, const GLbyte *v) noexcept
{
        opengl_functions::glVertexAttrib4Nbv(index, v);
}
inline void glVertexAttrib4Niv(GLuint index, const GLint *v) noexcept
{
        opengl_functions::glVertexAttrib4Niv(index, v);
}
inline void glVertexAttrib4Nsv(GLuint index, const GLshort *v) noexcept
{
        opengl_functions::glVertexAttrib4Nsv(index, v);
}
inline void glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) noexcept
{
        opengl_functions::glVertexAttrib4Nub(index, x, y, z, w);
}
inline void glVertexAttrib4Nubv(GLuint index, const GLubyte *v) noexcept
{
        opengl_functions::glVertexAttrib4Nubv(index, v);
}
inline void glVertexAttrib4Nuiv(GLuint index, const GLuint *v) noexcept
{
        opengl_functions::glVertexAttrib4Nuiv(index, v);
}
inline void glVertexAttrib4Nusv(GLuint index, const GLushort *v) noexcept
{
        opengl_functions::glVertexAttrib4Nusv(index, v);
}
inline void glVertexAttrib4bv(GLuint index, const GLbyte *v) noexcept
{
        opengl_functions::glVertexAttrib4bv(index, v);
}
inline void glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) noexcept
{
        opengl_functions::glVertexAttrib4d(index, x, y, z, w);
}
inline void glVertexAttrib4dv(GLuint index, const GLdouble *v) noexcept
{
        opengl_functions::glVertexAttrib4dv(index, v);
}
inline void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) noexcept
{
        opengl_functions::glVertexAttrib4f(index, x, y, z, w);
}
inline void glVertexAttrib4fv(GLuint index, const GLfloat *v) noexcept
{
        opengl_functions::glVertexAttrib4fv(index, v);
}
inline void glVertexAttrib4iv(GLuint index, const GLint *v) noexcept
{
        opengl_functions::glVertexAttrib4iv(index, v);
}
inline void glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w) noexcept
{
        opengl_functions::glVertexAttrib4s(index, x, y, z, w);
}
inline void glVertexAttrib4sv(GLuint index, const GLshort *v) noexcept
{
        opengl_functions::glVertexAttrib4sv(index, v);
}
inline void glVertexAttrib4ubv(GLuint index, const GLubyte *v) noexcept
{
        opengl_functions::glVertexAttrib4ubv(index, v);
}
inline void glVertexAttrib4uiv(GLuint index, const GLuint *v) noexcept
{
        opengl_functions::glVertexAttrib4uiv(index, v);
}
inline void glVertexAttrib4usv(GLuint index, const GLushort *v) noexcept
{
        opengl_functions::glVertexAttrib4usv(index, v);
}
inline void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) noexcept
{
        opengl_functions::glVertexAttribBinding(attribindex, bindingindex);
}
inline void glVertexAttribDivisor(GLuint index, GLuint divisor) noexcept
{
        opengl_functions::glVertexAttribDivisor(index, divisor);
}
inline void glVertexAttribDivisorARB(GLuint index, GLuint divisor) noexcept
{
        opengl_functions::glVertexAttribDivisorARB(index, divisor);
}
inline void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) noexcept
{
        opengl_functions::glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
}
inline void glVertexAttribFormatNV(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride) noexcept
{
        opengl_functions::glVertexAttribFormatNV(index, size, type, normalized, stride);
}
inline void glVertexAttribI1i(GLuint index, GLint x) noexcept
{
        opengl_functions::glVertexAttribI1i(index, x);
}
inline void glVertexAttribI1iv(GLuint index, const GLint *v) noexcept
{
        opengl_functions::glVertexAttribI1iv(index, v);
}
inline void glVertexAttribI1ui(GLuint index, GLuint x) noexcept
{
        opengl_functions::glVertexAttribI1ui(index, x);
}
inline void glVertexAttribI1uiv(GLuint index, const GLuint *v) noexcept
{
        opengl_functions::glVertexAttribI1uiv(index, v);
}
inline void glVertexAttribI2i(GLuint index, GLint x, GLint y) noexcept
{
        opengl_functions::glVertexAttribI2i(index, x, y);
}
inline void glVertexAttribI2iv(GLuint index, const GLint *v) noexcept
{
        opengl_functions::glVertexAttribI2iv(index, v);
}
inline void glVertexAttribI2ui(GLuint index, GLuint x, GLuint y) noexcept
{
        opengl_functions::glVertexAttribI2ui(index, x, y);
}
inline void glVertexAttribI2uiv(GLuint index, const GLuint *v) noexcept
{
        opengl_functions::glVertexAttribI2uiv(index, v);
}
inline void glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z) noexcept
{
        opengl_functions::glVertexAttribI3i(index, x, y, z);
}
inline void glVertexAttribI3iv(GLuint index, const GLint *v) noexcept
{
        opengl_functions::glVertexAttribI3iv(index, v);
}
inline void glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z) noexcept
{
        opengl_functions::glVertexAttribI3ui(index, x, y, z);
}
inline void glVertexAttribI3uiv(GLuint index, const GLuint *v) noexcept
{
        opengl_functions::glVertexAttribI3uiv(index, v);
}
inline void glVertexAttribI4bv(GLuint index, const GLbyte *v) noexcept
{
        opengl_functions::glVertexAttribI4bv(index, v);
}
inline void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) noexcept
{
        opengl_functions::glVertexAttribI4i(index, x, y, z, w);
}
inline void glVertexAttribI4iv(GLuint index, const GLint *v) noexcept
{
        opengl_functions::glVertexAttribI4iv(index, v);
}
inline void glVertexAttribI4sv(GLuint index, const GLshort *v) noexcept
{
        opengl_functions::glVertexAttribI4sv(index, v);
}
inline void glVertexAttribI4ubv(GLuint index, const GLubyte *v) noexcept
{
        opengl_functions::glVertexAttribI4ubv(index, v);
}
inline void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) noexcept
{
        opengl_functions::glVertexAttribI4ui(index, x, y, z, w);
}
inline void glVertexAttribI4uiv(GLuint index, const GLuint *v) noexcept
{
        opengl_functions::glVertexAttribI4uiv(index, v);
}
inline void glVertexAttribI4usv(GLuint index, const GLushort *v) noexcept
{
        opengl_functions::glVertexAttribI4usv(index, v);
}
inline void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) noexcept
{
        opengl_functions::glVertexAttribIFormat(attribindex, size, type, relativeoffset);
}
inline void glVertexAttribIFormatNV(GLuint index, GLint size, GLenum type, GLsizei stride) noexcept
{
        opengl_functions::glVertexAttribIFormatNV(index, size, type, stride);
}
inline void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) noexcept
{
        opengl_functions::glVertexAttribIPointer(index, size, type, stride, pointer);
}
inline void glVertexAttribL1d(GLuint index, GLdouble x) noexcept
{
        opengl_functions::glVertexAttribL1d(index, x);
}
inline void glVertexAttribL1dv(GLuint index, const GLdouble *v) noexcept
{
        opengl_functions::glVertexAttribL1dv(index, v);
}
inline void glVertexAttribL1i64NV(GLuint index, GLint64EXT x) noexcept
{
        opengl_functions::glVertexAttribL1i64NV(index, x);
}
inline void glVertexAttribL1i64vNV(GLuint index, const GLint64EXT *v) noexcept
{
        opengl_functions::glVertexAttribL1i64vNV(index, v);
}
inline void glVertexAttribL1ui64ARB(GLuint index, GLuint64EXT x) noexcept
{
        opengl_functions::glVertexAttribL1ui64ARB(index, x);
}
inline void glVertexAttribL1ui64NV(GLuint index, GLuint64EXT x) noexcept
{
        opengl_functions::glVertexAttribL1ui64NV(index, x);
}
inline void glVertexAttribL1ui64vARB(GLuint index, const GLuint64EXT *v) noexcept
{
        opengl_functions::glVertexAttribL1ui64vARB(index, v);
}
inline void glVertexAttribL1ui64vNV(GLuint index, const GLuint64EXT *v) noexcept
{
        opengl_functions::glVertexAttribL1ui64vNV(index, v);
}
inline void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y) noexcept
{
        opengl_functions::glVertexAttribL2d(index, x, y);
}
inline void glVertexAttribL2dv(GLuint index, const GLdouble *v) noexcept
{
        opengl_functions::glVertexAttribL2dv(index, v);
}
inline void glVertexAttribL2i64NV(GLuint index, GLint64EXT x, GLint64EXT y) noexcept
{
        opengl_functions::glVertexAttribL2i64NV(index, x, y);
}
inline void glVertexAttribL2i64vNV(GLuint index, const GLint64EXT *v) noexcept
{
        opengl_functions::glVertexAttribL2i64vNV(index, v);
}
inline void glVertexAttribL2ui64NV(GLuint index, GLuint64EXT x, GLuint64EXT y) noexcept
{
        opengl_functions::glVertexAttribL2ui64NV(index, x, y);
}
inline void glVertexAttribL2ui64vNV(GLuint index, const GLuint64EXT *v) noexcept
{
        opengl_functions::glVertexAttribL2ui64vNV(index, v);
}
inline void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) noexcept
{
        opengl_functions::glVertexAttribL3d(index, x, y, z);
}
inline void glVertexAttribL3dv(GLuint index, const GLdouble *v) noexcept
{
        opengl_functions::glVertexAttribL3dv(index, v);
}
inline void glVertexAttribL3i64NV(GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z) noexcept
{
        opengl_functions::glVertexAttribL3i64NV(index, x, y, z);
}
inline void glVertexAttribL3i64vNV(GLuint index, const GLint64EXT *v) noexcept
{
        opengl_functions::glVertexAttribL3i64vNV(index, v);
}
inline void glVertexAttribL3ui64NV(GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z) noexcept
{
        opengl_functions::glVertexAttribL3ui64NV(index, x, y, z);
}
inline void glVertexAttribL3ui64vNV(GLuint index, const GLuint64EXT *v) noexcept
{
        opengl_functions::glVertexAttribL3ui64vNV(index, v);
}
inline void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) noexcept
{
        opengl_functions::glVertexAttribL4d(index, x, y, z, w);
}
inline void glVertexAttribL4dv(GLuint index, const GLdouble *v) noexcept
{
        opengl_functions::glVertexAttribL4dv(index, v);
}
inline void glVertexAttribL4i64NV(GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w) noexcept
{
        opengl_functions::glVertexAttribL4i64NV(index, x, y, z, w);
}
inline void glVertexAttribL4i64vNV(GLuint index, const GLint64EXT *v) noexcept
{
        opengl_functions::glVertexAttribL4i64vNV(index, v);
}
inline void glVertexAttribL4ui64NV(GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w) noexcept
{
        opengl_functions::glVertexAttribL4ui64NV(index, x, y, z, w);
}
inline void glVertexAttribL4ui64vNV(GLuint index, const GLuint64EXT *v) noexcept
{
        opengl_functions::glVertexAttribL4ui64vNV(index, v);
}
inline void glVertexAttribLFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) noexcept
{
        opengl_functions::glVertexAttribLFormat(attribindex, size, type, relativeoffset);
}
inline void glVertexAttribLFormatNV(GLuint index, GLint size, GLenum type, GLsizei stride) noexcept
{
        opengl_functions::glVertexAttribLFormatNV(index, size, type, stride);
}
inline void glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) noexcept
{
        opengl_functions::glVertexAttribLPointer(index, size, type, stride, pointer);
}
inline void glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) noexcept
{
        opengl_functions::glVertexAttribP1ui(index, type, normalized, value);
}
inline void glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) noexcept
{
        opengl_functions::glVertexAttribP1uiv(index, type, normalized, value);
}
inline void glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) noexcept
{
        opengl_functions::glVertexAttribP2ui(index, type, normalized, value);
}
inline void glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) noexcept
{
        opengl_functions::glVertexAttribP2uiv(index, type, normalized, value);
}
inline void glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) noexcept
{
        opengl_functions::glVertexAttribP3ui(index, type, normalized, value);
}
inline void glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) noexcept
{
        opengl_functions::glVertexAttribP3uiv(index, type, normalized, value);
}
inline void glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) noexcept
{
        opengl_functions::glVertexAttribP4ui(index, type, normalized, value);
}
inline void glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) noexcept
{
        opengl_functions::glVertexAttribP4uiv(index, type, normalized, value);
}
inline void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) noexcept
{
        opengl_functions::glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}
inline void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) noexcept
{
        opengl_functions::glVertexBindingDivisor(bindingindex, divisor);
}
inline void glVertexFormatNV(GLint size, GLenum type, GLsizei stride) noexcept
{
        opengl_functions::glVertexFormatNV(size, type, stride);
}
inline void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) noexcept
{
        opengl_functions::glViewport(x, y, width, height);
}
inline void glViewportArrayv(GLuint first, GLsizei count, const GLfloat *v) noexcept
{
        opengl_functions::glViewportArrayv(first, count, v);
}
inline void glViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) noexcept
{
        opengl_functions::glViewportIndexedf(index, x, y, w, h);
}
inline void glViewportIndexedfv(GLuint index, const GLfloat *v) noexcept
{
        opengl_functions::glViewportIndexedfv(index, v);
}
inline void glViewportPositionWScaleNV(GLuint index, GLfloat xcoeff, GLfloat ycoeff) noexcept
{
        opengl_functions::glViewportPositionWScaleNV(index, xcoeff, ycoeff);
}
inline void glViewportSwizzleNV(GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew) noexcept
{
        opengl_functions::glViewportSwizzleNV(index, swizzlex, swizzley, swizzlez, swizzlew);
}
inline void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) noexcept
{
        opengl_functions::glWaitSync(sync, flags, timeout);
}
inline void glWaitVkSemaphoreNV(GLuint64 vkSemaphore) noexcept
{
        opengl_functions::glWaitVkSemaphoreNV(vkSemaphore);
}
inline void glWeightPathsNV(GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights) noexcept
{
        opengl_functions::glWeightPathsNV(resultPath, numPaths, paths, weights);
}
inline void glWindowRectanglesEXT(GLenum mode, GLsizei count, const GLint *box) noexcept
{
        opengl_functions::glWindowRectanglesEXT(mode, count, box);
}

// clang-format on

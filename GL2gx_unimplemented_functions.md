# glext.h #

## GL\_VERSION\_1\_3 ##
#### GL\_GLEXT\_PROTOTYPES ####
  * glActiveTexture'
  * glClientActiveTexture'

## GL\_VERSION\_2\_0 ##
#### GL\_GLEXT\_PROTOTYPES ####
  * glDrawBuffers'

## GL\_ARB\_shader\_objects ##
#### GL\_GLEXT\_PROTOTYPES ####
  * glDeleteObjectARB'

## GL\_EXT\_framebuffer\_object ##
#### GL\_GLEXT\_PROTOTYPES ####
  * glBindFramebufferEXT'
  * glBindRenderbufferEXT'
  * glCheckFramebufferStatusEXT'
  * glDeleteFramebuffersEXT'
  * glDeleteRenderbuffersEXT'
  * glFramebufferRenderbufferEXT'
  * glFramebufferTexture2DEXT'
  * glGenFramebuffersEXT'
  * glGenRenderbuffersEXT'
  * glGenerateMipmapEXT'
  * glRenderbufferStorageEXT'

# gl.h #
  * glDeleteLists' -- gl\_calllist.c(commented out)
  * glDrawBuffer' -- doesn't exist

# Version 1.2? #
gl.h contains this
  * define GL\_VERSION\_1\_1   1
  * define GL\_VERSION\_1\_2   1
gl\_extensions.c contains this
  * tatic const GLubyte vender[.md](.md) = "gl2gx";
  * tatic const GLubyte renderer[.md](.md) = "gx";
  * tatic const GLubyte version[.md](.md) = "1.2";
  * tatic const GLubyte extensions[.md](.md) = "GL\_EXT\_texture\_lod\_bias\nGL\_EXT\_texture\_filter\_anisotropic\nGL\_ARB\_multitexture\nGL\_EXT\_texture\_env\_add\nGL\_EXT\_texture\_env\_combine\nGL\_EXT\_draw\_range\_elements\nGL\_EXT\_bgra";

# Do I need to implement these other functions? or work around them? #
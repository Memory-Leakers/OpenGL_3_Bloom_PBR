Documentation requirements
The documentation has to be presented in a markdown file (README.md)
provided in the root folder of the repository in GitHub so that it can be seen from
a web browser. It will contain the following information:
● Names of the group members
● List of techniques implemented:
○ To show the effect of each technique, show a couple of renders from the
same point of view, with each technique enabled and disabled.
○ Explain how to enable / disable / configure the options you may have.
○ Include screenshots of the corresponding widgets when appropriate.
○ Name of shader files for every effect

# OpenGL_3_Bloom_PBR

## Group Members

Zhida Chen

Robert Recordà

## PBR
### Enabled/Disabled Pictures

![PBR 1](https://github.com/Memory-Leakers/OpenGL_3_Bloom_PBR/blob/main/WorkingDir/Fotos/0-1.jpg)

![PBR 2](https://github.com/Memory-Leakers/OpenGL_3_Bloom_PBR/blob/main/WorkingDir/Fotos/1-1.jpg)

### Options (Screenshots)
![PBR Options](https://github.com/Memory-Leakers/OpenGL_3_Bloom_PBR/blob/main/WorkingDir/Fotos/PBR-Options.jpg)

Inside the info tab/panel there is a toggle called PBR that turns it On or Off.

Also inside the same menu, there is an option called "Render Mode" that lets switch the render to any of the attachments and textures used by the PBR as well as visualizing the Render as Forward or Defrred.

The used textures are 
 - Albedo
 - Normal
 - Metallic
 - Specular
 - Ambient Oclussion
 - Emissive

### Shader files names

The PBR shader code is located inside:
 - RENDER_TO_BB.glsl
 - RENDER_TO_FB.glsl
 - FB_TO_BB.glsl

This files have a uniform bool variable called "usePBR" that when switched deactivated the basic lightning and turns on the PBR.


## Multipass - Bloom
### Pictures
To show the effect of each technique, show a couple of renders from the
same point of view, with each technique enabled and disabled.

### Options (Screenshots)
Explain how to enable / disable / configure the options you may have.

### Shader files names
Name of shader files for every effect

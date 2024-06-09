 

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

#### Default State
![PBR 1](https://github.com/Memory-Leakers/OpenGL_3_Bloom_PBR/blob/main/WorkingDir/Fotos/bloom1.png)

#### Step1 Brightes pixels
![PBR 1](https://github.com/Memory-Leakers/OpenGL_3_Bloom_PBR/blob/main/WorkingDir/Fotos/bloom2.png)

#### Step 2 Blur
![PBR 1](https://github.com/Memory-Leakers/OpenGL_3_Bloom_PBR/blob/main/WorkingDir/Fotos/bloom3.png)

#### Step 3 Bloom

  

### Options (Screenshots)
![PBR 1](https://github.com/Memory-Leakers/OpenGL_3_Bloom_PBR/blob/main/WorkingDir/Fotos/bloomOptions.png)

You can turn the bloom effect on or off with the following options:

-  Bloom Checkbox: Enables or disables the bloom effect.
- Bloom Test: This checkbox must be enabled to visualize the actual bloom effect.
- Brightness Threshold Scrollbar: Adjust the brightness threshold to control which parts of the scene contribute to the bloom effect.

  

### Shader files names

The PBR shader code is located inside:

- PASS_BLIT_BRIGHT.glsl

- BLUR.glsl

- BLOOM.glsl

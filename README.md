# Vulkan project

## Screenshots

![](./media/Screenshot.png)

This project is a Vulkan-based renderer developed to demonstrate and assemble key components of the Vulkan API. It serves as a foundational 
framework for understanding and experimenting with Vulkan's capabilities in graphics rendering.​

## Features
* Vulkan Integration: Utilizes Vulkan for high-performance, low-level graphics rendering.​

* Model Loading: Supports loading and rendering of 3D models.​
GitHub

* Shader Management: Implements shader compilation and management for customizable rendering effects.​
GitHub

* Texture Mapping: Incorporates texture loading and mapping to enhance visual detail.​

## Getting Started
Prerequisites:
* A system with Vulkan-compatible hardware and drivers.;
* C++14 compatible compiler.;
* Visual Studio 2022;
* x86-64 based CPU

## Building the Project:
### Additional Include Directories
In Visual Studio, right click on the project's name -> Properties -> C/C++ -> General.

Add

```
$(SolutionDir)/externals/GLFW/include;$(SolutionDir)/externals/GLM;<Path to your Vulkan installation>/Include;$(SolutionDir)/externals/ASSIMP/include;%(AdditionalIncludeDirectories)
```
to `Additional Include Directories`.

Note: you need to specify `<Path to your Vulkan installation>/Include` by yourself (it's in the line above), for example: `D:/Vulkan/Include`

### Additional Library Directories

In Visual Studio, right click on the project's name -> Properties -> Linker -> General,
Add
```
$(SolutionDir)/externals/GLFW/lib-vc2022;<Path to your Vulkan installation>/Lib;$(SolutionDir)/externals/ASSIMP/lib/Release;%(AdditionalLibraryDirectories)
```
to `Additional Library Directories`.

Note: you need to specify `<Path to your Vulkan installation>/Lib` by yourself (it's in the line above), for example: `D:/Vulkan/Lib`


### Additional Dependencies

In Visual Studio, right click on the project's name -> Properties -> Linker -> Input.

Make sure you have
```
vulkan-1.lib;glfw3.lib;assimp-vc143-mt.lib;%(AdditionalDependencies)
```
in `Additional Dependencies`.

At this point, you should be ready to go.

## License
This project is licensed under the MIT License. See the LICENSE file for details.
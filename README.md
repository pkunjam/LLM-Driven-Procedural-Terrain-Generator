## Overview

This project is a Procedural Terrain Generator implemented in C++ and OpenGL. It showcases various advanced rendering techniques, including Perlin noise-based terrain generation, normal mapping, water rendering, and a skybox for realistic visuals. 
The project is ideal for those interested in learning about computer graphics, procedural content generation, and modern OpenGL practices.

### Features

* **Procedural Terrain Generation:** Uses Perlin noise to generate realistic terrain with adjustable parameters like octaves, persistence, and frequency.
* **Normal Mapping:** Adds fine details to the terrain surface without increasing the mesh complexity.
* **Water Rendering:** Implements a reflective and refractive water surface with wave animations.
* **Skybox:** Surrounds the scene with a skybox for a fully immersive environment.
* **Basic Lighting:** Incorporates Phong lighting for ambient, diffuse, and specular reflections.
* **Camera Control:** An Arcball camera system allows for smooth panning, rotation, and zoom around the terrain.

### Project Structure

├── src/ <br>
&nbsp;&nbsp;&nbsp;├── main.cpp  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;        # Main entry point for the application <br>
&nbsp;&nbsp;&nbsp;├── PerlinNoise.cpp &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;   # Perlin noise implementation <br>
&nbsp;&nbsp;&nbsp;├── ArcballCamera.cpp &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  # Camera control implementation  <br>
&nbsp;&nbsp;&nbsp;└── shaders/ &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;          # Directory containing vertex and fragment shaders <br> 
├── textures/  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;            # Directory for textures used in the project <br> 
├── CMakeLists.txt  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;      # CMake build script <br> 
└── README.md     &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;       # Project documentation <br> 

## Getting Started

### Prerequisites

* **C++ Compiler:** Ensure you have a modern C++ compiler installed (GCC, Clang, or MSVC).
* **CMake:** Required to build the project.
* **GLFW:** For window management and OpenGL context creation.
* **GLAD:** For loading OpenGL functions.
* **stb_image:** For loading image files (e.g., textures).

## Instatllation

1. Clone the repository

```sh
git clone https://github.com/pkunjam/procedural-terrain-generator.git
cd procedural-terrain-generator 
```
2. Build the project

```sh
mkdir build
cd build
cmake ..
make
```
3. Run the Executable

```sh
./ProceduralTerrain
```

## Usage

**Camera Controls:**
* **Left Mouse Button:** Rotate the camera around the terrain.
* **Right Mouse Button:** Pan the camera.
* **Scroll Wheel:** Zoom in and out.

**Terrain Parameters:** Adjust the terrain parameters (octaves, persistence, etc.) directly in the main.cpp file to see how different settings affect the terrain generation.

## Contributing

Contributions are what makes the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## License

This project is licensed under the GNU General Public License v3.0 License. See `LICENSE.txt` for more details.

## Acknowledgments

[Learn OpenGL by Joey de Vries](https://learnopengl.com/Introduction)

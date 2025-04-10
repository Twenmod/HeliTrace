This is a raytraced on rails shooter made in 8 weeks for BLOCK C of the Programming track at Breda University of Aplied Sciences 

![1 _Basics_dnCud6aRM0](https://github.com/user-attachments/assets/74e2f05b-d0d2-42e1-8945-ee08a2eabb7d)

# running
1) Open "tmpl_2025-rt.sln"
2) make sure you select the correct startup project i.e *1. Basics* and not UnitTests
3) Make sure you are running on release
4) build and run
## libraries
Some external libraries are required for this project
- GLFW
- glad
- glm
- stb_image
- libdl (for miniaudio.h)

# gameplay
Your goal is to destroy the 6g tower on top of the building, Kill as many enemies as you can on the way there
Score is based on enemies killed but you get a bonus for killing them using

## controls
[LEFT_MOUSE] Shoot

[RIGHT_MOUSE] Aim

[ESC] for pause menu

[Q] for quit

[R] to restart

[(LEFT/RIGHT)MOUSE] for UI

# settings/configurables
Most easy to configure constants like render resolution or render settings can be configured inside the ".\template\common.h" file

This includes an *ILO* mode which sets up the 256x212 res, enables stochastic fresnel and reduces some volumetrics accuracy so the project definetely runs at the minimum fps on most systems.

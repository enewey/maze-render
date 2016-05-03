Maze Renderer
======
Developed in C++ and OpenGL for CS 5610 Interactive Computer Graphics at the University of Utah, Spring 2016.
>The project was developed alongside my Maze Generator project: [github here](https://github.com/enewey/maze-generator)

Project Description
------
This project aims to render a maze-like series of walls and floors in a first-person perspective. The walls use bump-mapped textures, and along the ground are randomly placed billboard sprites that resemble grass. The floor is also made to look like a big pool of reflective water, that reflects the walls/grass and distorts the reflection with a ripple effect.

###It's a "Game".

The renderer also serves as sort of a "game". In this context, "game" means:

1. There is a starting point.
2. There is collision detection to prevent walking through the walls
3. There is an ending point (designated by a "trophy" billboard sprite).

There is not much else to the "game" portion of things. No win condition, really. It just makes it a bit more interesting.

###Grass rendering

The grass rendering algorithm goes like so:

* For every piece of floor (provided by the maze-generator), we render X sprites of grass.
* The grass sprites are places pseudo-randomly across that square of floor. It uses a non-uniform distribution method (i.e. totally random).
* That's it.

Currently there are about 36 sprites of grass rendered per piece of floor.

###Reflection/Water rendering

To get the reflective floor:

* First a framebuffer object is created.
* The framebuffer is then set to render to a texture.
* We then draw the walls, grass, and trophy all upside-down from their normal position.
* The current framebuffer is then swapped to the default framebuffer.

These steps allow us to get a "reflection" of the walls/sprites into a texture. From here, we draw the floors and pass in our new texture, setting the uv-coordinates for the texture as the on-screen coordinates of each fragment (gl_FragCoord.xy).

To get the ripple effect, I use a simple sin wave pattern to displace the x-position of the texture coordinate: `tc.x + (sin((tc.y + current_time) * frequency) * amplitude)`. I use the y-component of the texture coordinate to create the illusion that the ripples are smaller the further away the floor is. It's a personal preference, really.

###Screenshots

![Screenshot 1](final_screen_0.png?raw=true)
![Screenshot 2](final_screen_1.png?raw=true)
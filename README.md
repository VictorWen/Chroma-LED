# Chroma

This is Chroma, an LED controls system built to calculate and deliver fast, customizable animated effects to distributed LED devices.
Chroma uses a custom scripting language called ChromaScript for non-programmers to easily customize animations.
For programmers, a modular object-oriented design allows for easy additions of custom commands and effects.
Chroma also includes Disco, a network system that manages LED device configurations and delivery of LED pixel data across the network.

**Table of Contents**
* Installation
* Chroma Controller
  * Combining Effects
  * Stacking
  * Layering
* ChromaScript
  * Data Values
  * Commands
  * Command Stacking
  * Variables
  * Functions
  * Scripts
* Disco

## Installation

### Prerequisites

* Boost
* httpserver
* nolhmann_json

These should be put into the libraries/ folder

### Compiling

*Windows*

`make win`

*Unix*

`make chroma`

## Chroma Controller

The basic unit of control for the Chroma controller is an Effect.
An Effect is a series of instructions detail when and how each LED pixel of a device should be lit and colored.
For examples, the `rgb` Effect colors the entire LED device with a specified RGB value.
While, the `rainbow` Effect colors the devices with a rainbow from red to purple.
Effects also include animation Effects, such as `slide` which shifts the color of the device at a specified rate.

> More complicated effects can be achieved via the Particle Physics system.

## Combining Effects

Effects are combinable in two ways:
1. by stacking
2. by layering

### Stacking

For sake of simplicity, we can split Effects into two categories, Color Effects and Animation Effects.
Color Effects would include Effects like `rgb` and `rainbow`.
They are static and *do not* change with time.
Animation Effects, on the other hand, would include Effects like `slide`.
They are dynamic and *do* change with time.

However for an animation to change with time, they need to know what they are changing.
That's where Color Effects come back.
An Animation Effect takes in Color Effects, along with other parameters, and uses that to determine the animation pattern.
For examples, a `slide` with a `rainbow` will move the rainbow colors around the LED device.

For further information see ChromaScript Stacking

### Layering

A device can display multiple Effects at once using layers.
The Chroma controller keeps track of multiple active Effects by keeping them each in separate layers.
Since colors are defined with RGBA values, layers are merged via alpha compositing.
It is possible to create unique and complicated animations using several layers.

## ChromaScript

ChromaScript is the custom scripting language designed to detail Effects to the Chroma controller.

### Data Values

Chroma uses the following data types

**Numbers**
```ChromaScript
1
12.3
.12
-32
-12.32
-.32
```

**Strings**
```ChromaScript
"Hello, World!"
""
```

**Objects**
Objects can only be generated from Commands, as discussed below
```ChromaScript
rainbow
(rainbow)
(pbody 1 2 3)
```

**Lists**
```ChromaScript
[1 2 3]
["A" "B" "C"]
[]
[
    1
    2
    3
]
[
    rainbow 
    (pbody 1 2 3)
]
```

### Commands 

The basic unit of ChromaScript is a Command.
A Command is just a word that accepts some parameters and produces an Effect to the Chroma controller.
Parameters are positional, meaning the order of the parameters matter.
Parameters can be required or optional, and can be extensible (variadic)

The following are examples of Commands and them being called

```ChromaScript
rgb 255 0 0 # Calls the rgb command, producing a red coloring Effect
(rgb 255 0 0) # Another way of the calling the above
rainbow # Calls the rainbow command, producing a rainbow coloring Effect
(rainbow) # Another way of calling rainbow
(slide (rainbow) 10) # Calls the slide and rainbow commands, producing a sliding rainbow animation Effect (with a period of 10 seconds)
(slide rainbow 10) # Another way of calling the above
```

### Command Stacking

As shown above, Commands can be stacked to produce stacked commands.
This is a result of a Command taking an Effect as a parameter

```ChromaScript
(rainbow) # produces the rainbow Effect
(slide (rainbow) 10) # calls rainbow to create the rainbow Effect, which is then passed into the slide Command

(rgb 255 0 0) # produces the color red
(rgb 0 0 255) # produces the color blue
(gradient (rgb 255 0 0) (rgb 0 0 255)) # produces a gradient from red to blue
(slide # Commands can be called in a multi-line fashion
    (gradient
        (rgb 255 0 0)
        (rgb 0 0 255)
    )
    10
) # slides a gradient from red to blue across the device with a period of 10 seconds
```

### Variables

Variables are names/identifiers that store data values to be used later.

Variables are defined and set using the `let` keyword.
The format is: `let NAME = VALUE`

```ChromaScript
let period = 10
(slide rainbow period)

let RED = rgb 255 0 0
let BLUE = rgb 0 0 255
let RBGradient = gradient RED BLUE
(slide RBGradient period)
```

Variables are immediately evaluated and save a reference to the data value they store.

```ChromaScript
let A = (slide rainbow 10)
A # runs the sliding rainbow effect
(rgb 255 0 0) # changes the current effect to red
# wait some time
A # changes the current effect back the sliding rainbow effect, continuing from where it left off
```

### Functions

Functions are basically variables that save code to be ran later.
They are defined using the `func` keyword and called exactly the same as Commands.

> Internally, Commands are just functions

The format to define is: `func NAME PARAM1 PARAM2 ... PARAMN = CODE`

```ChromaScript
let RED = rgb 255 0 0
let BLUE = rgb 0 0 255

func slideGradient color1 color2 = (slide
    (gradient
        color1
        color2
    )
)

(slideGradient RED BLUE)
```

### Scripts

Beyond inputting Commands into the Chroma CLI, Commands can be read from '.chroma' script files.
Simply write the Commands into a '.chroma' file separated by newlines, then use the 'read' keyword to load the script

example.chroma
```ChromaScript
let RED = rgb 255 0 0
let BLUE = rgb 0 0 255

func slideGradient color1 color2 = (slide
    (gradient
        color1
        color2
    )
)

rainbow
```

Chroma CLI
```ChromaScript
read "example.chroma" # sets the effect to the rainbow effect
(slideGradient RED BLUE) # calls the slideGradient function defined in examples.chroma
```

## Disco
WIP
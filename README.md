# fooblox
A hackathon project from undergrad

I did this project in 2015 as part of a 48 hour hackathon with over 1000 hackers. This project was a finalist and selected as a staff pick by Devpost.

Hackathon entry/writeup: https://devpost.com/software/foo-blox

![image](https://challengepost-s3-challengepost.netdna-ssl.com/photos/production/software_photos/000/311/945/datas/gallery.jpg)

Basically what this project does is let you write python code using blocks. The idea was to make a fun coding learning experience for kids.

How it works:
- There is a loop that reads video input from a webcam
- OpenCV processes the input and detects blocks based on their color
- The position and types of the blocks are translated into a python string
- The python code is executed and the output is collected
- The output is passed to the frontend (using awesomium)
- The output shows a diff of the correct solution
- When the correct solution is reached, the level advances.

All of the code is written in C++. I used a library to run python from C++ and a library to allow for html ui.

Due to working solo and only having 48 hours, the code is very ugly :)
Most of the game logic is in `main.cpp` and most of the openCV logic is in `main.h`
The frontend logic is in `app.html`

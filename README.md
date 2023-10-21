# CS350-Thermostat-embedded-system 

This repository holds my completed work for a thermostat system that utilizes a microcontroller to simulate the functionality of the thermostat. It also includes a completed State Machine Diagram and Project Reflection and Recommendation report, giving a description of the project as well as my proposal for the second phase of the project, connecting the system to the cloud. 

## Project Reflection 

#### Project Summary 

This goal of this project was to develop an embedded system prototype that would read the ambient room temperature and turn a heater on if the temperature was lower then the desired temperature setting. The system should also watch for button presses that indicate raising or lowering the temperature set point, report out the current temp, set temp, status of the heater, and the amount of time in seconds from the last board reset.  

Each of these were to be captured at different periods, with the button press check occurring every 200ms, checking the current temp and comparing to set temp every 500ms, and reporting out and switching the heater on or off every 1000ms.  

#### Successes 

I believe that I was able to excel in multiple areas of the project. The first is the development of the state machine within the program. Utilizing best practices, I was able to create a simple, effective, and low-resource solution to address the project requirements. I also developed a well-defined, structured, and informative report to provide to the client, showcasing the system, its functionality, and recommendations for the next phase of the project. 

#### Opportunities 

As with all things, there are always improvements that can be executed, and efficiencies gained. For this project, I believe there are still improvements I could make to the code base to increase error-handling situations. This would ensure that the system would gracefully react to an error, providing critical information to the user, and improving the usability of the system. 

#### Tools and/or Resources added to my kit 

Throughout this course, I have been able to utilize many new tools and resources to accomplish the goals of the project. Some that will be key for me include CCS (Code Composer Studio) as I continue to work on creating programs to be utilized in embedded systems. While this is typically utilized in conjunction with TI (Texas Instruments) boards, I do have one that I can utilize for testing purposes to continue to build my skills.  

#### Transferable skills from this project 

I think the biggest transferable skill that I have developed while working on this project would be the diagramming skills gained through the creation of State Machine diagrams. While this is specific in nature, I believe that the theory behind its development can be utilized in many areas of software engineering. 

#### Project Maintainability, Readability, and Adaptability 

To ensure that the project that I developed was maintainable, I utilized OOP best practices when developing the code, ensuring that the code meets those standards. To improve readability throughout the program, I have utilized standard naming conventions and implemented robust and informative comments throughout. The program is also adaptable, done so by breaking out each component of the system to be standalone, allowing for their use in other systems quickly and effectively. 

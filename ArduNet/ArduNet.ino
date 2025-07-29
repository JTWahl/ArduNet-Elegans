//TODO: design some test circuits and ROM... verify this is crunching numbers right

/**
 *                    STRETCH GOALS
 *  -reduce global variable memory use to make room for hebbian learning
 *  -save weight adjustments between on-off cycles? eeprom?
 *  -finish the simulation and output screen function
 */

#include "neuralROM.h"            //import libraries
#include "sprites.h"
#include "bit_array.h"
#include <Arduboy2.h>
#include <stdlib.h>

Arduboy2 arduboy;                 //create arduboy object

const uint8_t threshold = 1;      //threshold for activation function
const uint16_t maxSynapse = 65;
uint16_t currentID = 0;           //interface variable to indicate which neuron is being analyzed
uint16_t numSynapses = 0;         //interface variable to indicate number of synapses connected
uint16_t preID = 0;               //interface variable to indicate which neuron is being analyzed
uint8_t sense = 0;                //interface variable to indicate which sense is activated
uint8_t posCount = 0;             //interface variable to indicate which sense is being looked at
uint8_t lastScreen = 0;           //interface variable to indicate last screen before button press
uint8_t buttonHold = 0;            //interface variable to hold number of iterations button is held
int8_t synWeight = 0;             //interface variable to indicate the weight of a given synapse
bool startFlag = true;            //interface flag for title screen to play
float vaRatio = 0;                //muscle ratios for interface printout
float vbRatio = 0;
float daRatio = 0;
float dbRatio = 0;
uint8_t counter = 0;


//              non-learning neurons??
//260, 264, 265, 266, 267, 268, 269, 270, 271, 261, 262, 263            
//272, 275, 276, 277, 278, 279, 280, 281, 282, 273, 274            
//88, 89, 90, 91, 92, 93, 94, 95, 96            
//97, 98, 99, 100, 101, 102, 103 

uint16_t preSynapticNeuronList[maxSynapse];  //interface array to hold all the different presynaptic neurons
//int8_t learningArray[302];          //an array that, for each neuron, holds its firing history
BitArray<302> outputList;     //list of neurons
BitArray<302> nextOutputList; //buffer to solve conflicting time differentials in firing

struct Neuron {
  int16_t cellID;
  int16_t inputLen;
  int16_t inputs[maxSynapse];
  int16_t weights[maxSynapse];    //global neuron struct for neuron 'n'
} n;                              


//setup function
void setup() {
  arduboy.begin();
  arduboy.setFrameRate(25);
}

//primary loop function
void loop() {
  if (!(arduboy.nextFrame())) {  //verify that runs properly
    return;
  }
  
  doTitleScreen();

  arduboy.pollButtons();        //get buttons pressed and send to function to process them
  doButtons();

  activationFunction();         //do the main calculation of the connectome
  delay(50);

  //go to the proper screens based on the direction button last clicked
  if (arduboy.justPressed(UP_BUTTON) || startFlag) doMatrixScreen();
  //if (arduboy.justPressed(DOWN_BUTTON))  doOutputScreen();
  if (arduboy.justPressed(LEFT_BUTTON)) doDiagnosticScreen();
  if (arduboy.justPressed(RIGHT_BUTTON)) doInputScreen();

  arduboy.display();
  
  startFlag = false;            //set the flag off; only do the title screen once
}

/**
 * function to run the diagnostic screen; a screen that shows details of synapses
 * and allows the user to make changes
 */
void doDiagnosticScreen() {
      lastScreen = 3;

      arduboy.clear();    
      arduboy.setCursor(10, 10);
      arduboy.print("# ");
      arduboy.print(currentID);
      arduboy.print(" Syn: ");
      arduboy.print(numSynapses); 
      arduboy.setCursor(10, 20);
      arduboy.print("Weight: ");
      arduboy.print(synWeight);
      arduboy.setCursor(10, 30);
      arduboy.print("Pre ID: ");
      arduboy.print(preID);
      arduboy.setCursor(10, 40);
      arduboy.print("postID: ");
      arduboy.print(currentID);

      if (outputList[currentID]) Sprites::drawOverwrite(85, 30, perceptronON, 0);
      if (!outputList[currentID]) Sprites::drawOverwrite(85, 30, perceptronOFF, 0);

      arduboy.display();
}

/**
 * Function to activate the input screen where the user can choose a sensory
 * input modality to interface with
 */
void doInputScreen() {
  //if pressed right draw inputs screen
      lastScreen = 4;
      
      arduboy.clear();
      arduboy.setCursor(10,0);
      arduboy.print("Select Input Type: ");
      arduboy.setCursor(10,20);
      arduboy.print("-Nose Touch");
      arduboy.setCursor(10,30);
      arduboy.print("-Chemorepulsion");
      arduboy.setCursor(10,40);
      arduboy.print("-Chemoattraction");

        if (posCount == 0) {
          arduboy.drawPixel(0, 33, WHITE);
          arduboy.drawPixel(1, 33, WHITE);
          arduboy.drawPixel(2, 33, WHITE);
          arduboy.drawPixel(1, 32, WHITE);
          arduboy.drawPixel(1, 34, WHITE);
          arduboy.display();
        } else if (posCount == 1) {
          arduboy.drawPixel(0, 43, WHITE);
          arduboy.drawPixel(1, 43, WHITE);
          arduboy.drawPixel(2, 43, WHITE);
          arduboy.drawPixel(1, 42, WHITE);
          arduboy.drawPixel(1, 44, WHITE);
          arduboy.display();
        } else if (posCount == 2) {
          arduboy.drawPixel(0, 23, WHITE);
          arduboy.drawPixel(1, 23, WHITE);
          arduboy.drawPixel(2, 23, WHITE);
          arduboy.drawPixel(1, 22, WHITE);
          arduboy.drawPixel(1, 24, WHITE);
          arduboy.display();
        }
}

/**
 * function to query which buttons are pressed, setting up proper screen transitions
 */
void doButtons() {
  //if last screen was the matrix screen
  if (lastScreen == 1) {
    if (arduboy.justPressed(A_BUTTON) && arduboy.justPressed(B_BUTTON)) {
      if (sense == 0) doNoseTouch(); 
      if (sense == 1) doChemorepulsion();
      if (sense == 2) doChemoattraction();
    }

    if (arduboy.justPressed(A_BUTTON)) {
      if (currentID < 302 - 1) {
        currentID++;
      } else {
        currentID = 0;
      }
    }

    if (arduboy.justPressed(B_BUTTON)) {
      if (currentID < 302 - 1) {
        if (currentID + 10 > 302 - 1) {
          currentID = 301;
        } else {
          currentID += 10;
        }
      } else {
        currentID = 0;
      }
    }

    doMatrixScreen();
  }

  //if last screen was the Output Screen
  /*if (lastScreen == 2) {
     if (arduboy.justPressed(A_BUTTON)) {
        simView = 1;
     }
     if (arduboy.justPressed(B_BUTTON)) {
        simView = 0;
     }
  }*/

  //if last screen was the diagnostic screen
  if (lastScreen == 3) {
    if (arduboy.justPressed(A_BUTTON)) {
      if (counter < numSynapses - 1) {
        counter++;
      } else {
        counter = 0;
      }
      //synWeight = 0;
      preID = preSynapticNeuronList[counter];
    }

    if (arduboy.justPressed(B_BUTTON)) {
      if (counter > 0) {
        counter--;   
      } else {
        counter = n.inputLen;
      }
      //synWeight = 0;
      preID = preSynapticNeuronList[counter];
    }

    doDiagnosticScreen();
  }
  
  //if last screen was the input select screen
  if (lastScreen == 4) {
    if (arduboy.justPressed(A_BUTTON)) {
       posCount++;
       if (posCount > 2) posCount = 0;
  
       if (posCount == 0) {
         sense = 0;
       }
            
       if (posCount == 1) {
         sense = 1;
       }
          
       if (posCount == 2) {
         sense = 2;
       }            
    }

    doInputScreen();
  }  

  arduboy.display();
}

/**
 * Function that displays the main title screen
 */
void doTitleScreen() {
  if (startFlag) {
    //clear the screen then write app name
    arduboy.clear();
    arduboy.setCursor(15, 10);
    arduboy.print(F("-ArduNet Elegans-"));
    arduboy.setCursor(10, 50);
    arduboy.display();
    delay(5000);  
  }
}

/**
 * Function to show the output of muscle movement ratios to the user
 */
/*void doOutputScreen() {
  //if pressed left draw output screen
      lastScreen = 2;
      
      //draw graphic of worm for cell output
      //draw grid of interneurons
      //draw muscle ratios
      //draw direction of movement
    
      arduboy.clear();    
      arduboy.setCursor(0, 0);

      if (!simView) {
          //print the motor ratios for the motor cells
          uint8_t VAcount = 12;
          uint8_t VBcount = 11;
          uint8_t DAcount = 9;
          uint8_t DBcount = 7;
          uint8_t VAsum = 0;
          uint8_t VBsum = 0;
          uint8_t DAsum = 0;
          uint8_t DBsum = 0;

          //backward ventral locomotion motor neurons
          const uint16_t motorNeuronAVentral[VAcount] = {
            VA1, VA2, VA3, VA4, VA5, VA6, VA7, VA8, VA9, VA10, VA11, VA12
          };
  
          //forward ventral locomotion motor neurons
          const uint16_t motorNeuronBVentral[VBcount] = {
            VB1, VB2, VB3, VB4, VB5, VB6, VB7, VB8, VB9, VB10, VB11
          };
  
          //backward dorsal locomotion motor neurons
          const uint16_t motorNeuronADorsal[DAcount] = {
            DA1, DA2, DA3, DA4, DA5, DA6, DA7, DA8, DA9
          };
  
          //forward dorsal locomotion motor neurons
          const uint16_t motorNeuronBDorsal[DBcount] = {
            DB1, DB2, DB3, DB4, DB5, DB6, DB7
          };
  
  
          for (uint16_t i = 0; i < VAcount; i++) {
              if (outputList[i]) {
                  VAsum++;
              }
          }
  
          for (uint16_t i = 0; i < VBcount; i++) {
              if (outputList[i]) {
                  VBsum++;
              }
          }
  
          for (uint16_t i = 0; i < DAcount; i++) {
              if (outputList[i]) {
                  DAsum++;
              }
          }
  
          for (uint16_t i = 0; i < DBcount; i++) {
              if (outputList[i]) {
                  DBsum++;
             }
          }
  
          vaRatio = float(VAsum)/float(VAcount);
          vbRatio = float(VBsum)/float(VBcount);
          daRatio = float(DAsum)/float(DAcount);
          dbRatio = float(DBsum)/float(DBcount);
  
          arduboy.setCursor(10, 20);
          arduboy.print("VA Ratio: ");
          arduboy.print(vaRatio);
          arduboy.setCursor(10, 30);    
          arduboy.print("VB Ratio: ");
          arduboy.print(vbRatio);
          arduboy.setCursor(10, 40);
          arduboy.print("DA Ratio: ");
          arduboy.print(daRatio);
          arduboy.setCursor(10, 50);
          arduboy.print("DB Ratio: ");
          arduboy.print(dbRatio);
      
          printMovementDir(5, 10);
      } else {
        arduboy.drawRoundRect(0, 0, 128, 64, 3);         //border around screen (wall)
//TODO: finish simulation here
        //demarcation on left for temp gradients
        //demarcation on right for O2/CO2 gradients
        //single line showing surface and light exposure activation
        //screen border activates soft touch
        //when A+B is hit food is randomly generated, releasing chemical gradient
        //when food is touched it stimulates gustatory neurons
        //emote icon that follows worm and queries different neurons for output

      /if (vaRatio + daRatio > vbRatio + dbRatio) {    //worm is moving backward

        } else {                                        //worm is moving forward

        }
  
        if (vaRatio + vbRatio > daRatio + dbRatio) {    //worm is moving left

        } else {                                        //worm is moving right

        }
      }  
    }   
    
    arduboy.display();
}*/

/**
 * A function that displays the main "matrix" screen to the user, allows them to activate
 * a chosen sensory modality, and to view network activation patterns in the matrix
 */
void doMatrixScreen() {      
  lastScreen = 1;
  
  arduboy.clear();
  arduboy.setCursor(5, 5);
  arduboy.print("NEURAL GRID: ");
  arduboy.setCursor(5, 40);
  arduboy.print("CURRENT CELL ID: ");
  arduboy.print(currentID);
  printMovementDir(5, 50);
        
  uint8_t gridWidth = 17;
  uint8_t gridHeight = 18;
  uint16_t xPos = 80;
  uint16_t yPos = 1;
        
  //draw border on grid of cells
  arduboy.drawRect(xPos, yPos, (gridWidth + 1)*2, (gridHeight + 1)*2);

  int neuronCounter = 0;
      
  //draw grid of cells, blacking out current cell ID
  for (uint16_t x = 1; x <= (gridWidth*2); x++) {
    for (uint16_t y = 1; y <= (gridHeight*2); y++) {
      //for each neuron get the output state and draw a pixel accordingly               
      if (neuronCounter < 302) {
        if (outputList[neuronCounter]) {
          arduboy.drawPixel(x + xPos, y + yPos, WHITE);
          arduboy.drawPixel(x + xPos + 1, y + yPos, WHITE);
          arduboy.drawPixel(x + xPos, y + yPos + 1, WHITE);
          arduboy.drawPixel(x + xPos + 1, y + yPos + 1, WHITE);        
        }
      }
      neuronCounter++;

      y++;
    }

    x++;
  }
}

/******************************************SENSES********************************************/
/**
 * Do nose touch sense
 */
void doNoseTouch() {
    for (uint16_t id = 0; id < 302; id++) {
            //if nosetouch neuron is in cellular matrix then set output to true
            if (id == 113 || id == 114) {
                outputList[id] = true;
            }
        }
} //FLP

/**
 * Do chemorepulsive sense
 */
void doChemorepulsion() {
    for (uint16_t id = 0; id < 302; id++) {
            //if nosetouch neuron is in cellular matrix then set output to true
            if (id == 161 || id == 162 || id == 163|| id == 164) {
                outputList[id] = true;
            }
        }
} //PHA, PHB

/**
 * Do chemoattractive sense
 */
void doChemoattraction() {
    for (uint16_t id = 0; id < 302; id++) {
            //if nosetouch neuron is in cellular matrix then set output to true
            if (id == 39 || id == 40) {
                outputList[id] = true;
            }
        }
} //ASE

/*void doGentleNoseTouch() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 153 || id == 154 || id == 155 || id == 156 ||
                id == 125 || id == 126 || id == 127 || id == 128 || 
                id == 129 || id == 130 || id == 113 || id == 114) {
                outputList[id] = true;
            }
        }
}     //OLQ, IL1, FLP

void doGentlePosBodyTouch() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 167 || id == 168) {
                outputList[id] = true;
            }
        }
}  //PLM

void doGentleAntBodyTouch() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 23 || id == 24 || id == 71) {
                outputList[id] = true;
            }
        }
}  //ALM, AVM

void doHarshBodyTouch() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 174 || id == 174) {
                outputList[id] = true;
            }
        }
}      //PVD

void doHarshNoseTouch() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 113 || id == 114 || id == 43 || id == 44) {
                outputList[id] = true;
            }
        }
}      //FLP, ASH

void doOsmoticResponse() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 43 || id == 44 || id == 6 || id == 7) {
                outputList[id] = true;
            }
        }
}     //Educated guess based on protein presence on neurons: ASH, ADL

void doTextureSense() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 84 || id == 85 || id == 86 || id == 87 || 
                id == 2 || id == 3 || id == 159 || id == 160) {
                outputList[id] = true;
            }
        }
}        //CEP, ADE, PDE

void doChemoattraction() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 39 || id == 40 || id == 4 || id == 5 || 
                id == 41 || id == 42 || id == 45 || id == 46 || 
                id == 47 || id == 48 || id == 49 || id == 50) {
                outputList[id] = true;
            }
        }
}     //ASE (primary), ADF, ASG, ASI, ASJ, ASK

void doChemorepulsion() {
    for (uint16_t id = 0; id < 302; id++) {
            if (id == 43 || id == 44 || id == 6 || id == 7 || 
                id == 49 || id == 50 || id == 39 || id == 40) {
                outputList[id] = true;
            }
        }
  }      //ASH (primary), ADL, ASK, ASE

void doCoolingResponse() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 76 || id == 77) {
                outputList[id] = true;
            }
        }
}     //AWC

void doHeatingResponse() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 8 || id == 9) {
                outputList[id] = true;
            }
        }
}     //AFD

void doNoxiousHeatResponse() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 8 || id == 9 || id == 113 || id == 114 || 
                id == 165 || id == 166) {
                outputList[id] = true;
            }
        }
} //AFD (head), FLP (head), and PHC (tail)

void doNoxiousColdResponse() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 174 || id == 175) {
                outputList[id] = true;
            }
        }
}      //PVD

void doPhotosensation() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 63) {
                outputList[id] = true;
            }
        }
}      //Educated guess based on protein presence on neurons: AVG

void doOxygenSensation() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 27 || id == 171 || id == 254 || id == 255 ||
                id == 43 || id == 44) {
                outputList[id] = true;
            }
        }
}     //AQR, PQR, URX, ASH (very likely based on protein presence)

void doCO2Sensation() {
      for (uint16_t id = 0; id < 302; id++) {
            if (id == 8 || id == 9 || id == 78 || id == 79 || 
                id == 39 || id == 40) {
                outputList[id] = true;
            }
        }
}      //AFD, BAG, and ASE
*/
/*************************************SIMULATION FUNCTIONS***************************************/
/**
 * The function that reads in the neural rom into a format that is able to be read 
 * by the activation function and rest of the program
 */
void matrixToNeuron(uint16_t cellID) {
  uint16_t index = 0;

  // Skip to the correct neuron's data
  for (uint16_t i = 0; i < cellID; i++) {
    int16_t skipLen = pgm_read_word(&NEURAL_ROM[index]);     //read the value of the first neuron's input Len
    index += 1 + skipLen + skipLen;       //add double that value plus one, to skip the entire neuron entry
  }

  n.inputLen = pgm_read_word(&NEURAL_ROM[index]);
  index++;

  // Read neuron inputs
  for (uint8_t i = 0; i < n.inputLen; i++) {
    n.inputs[i] = pgm_read_word(&NEURAL_ROM[index++]);
  }

  // Read neuron weights
  for (uint8_t i = 0; i < n.inputLen; i++) {
    n.weights[i] = pgm_read_word(&NEURAL_ROM[index++]);
  }
}

/**
 *The activation function is the main simulation, it calculates all the next ticks of the connectome
 * and then sets the next tick to the current one when each has been individually calculated
 */
void activationFunction() {  
  //calculate next output for all neurons using the current output list
  for (uint16_t i = 0; i < 302; i++) {
    matrixToNeuron(i);      //fill the neuron struct with the information of the ith neuron
    int32_t sum = 0;

    for (uint8_t j = 0; j < n.inputLen; j++) {        //loop over every presynaptic neuron
      if (i == currentID) {
        numSynapses = n.inputLen;   //store some more info about these synapses
        preSynapticNeuronList[j] = n.inputs[j];
        
        if (n.inputs[j] == preID) {
          synWeight = n.weights[j];
        }
      }

      sum += n.weights[j] * outputList[n.inputs[j]];  //do the summation calculation on the neuron
      //sum += learningArray[i];                        //add hebbian learning and LTD
    }

    if (sum >= threshold) {                           //note it as being true or false
      nextOutputList[i] = true;
      
      /*if (learningArray[i] < 0) {                     //hebbian learning
        learningArray[i] = 0;
      } else {
        learningArray[i]++;
      }*/
    } else {
      nextOutputList[i] = false;
      
      /*if (learningArray[i] > 0) {
        learningArray[i] = 0;
      } else {
        learningArray[i]--;
      }*/
    }
  }

  for (int16_t i = 0; i < 302; i++) {         //flush buffer
    outputList[i] = nextOutputList[i];
  }
}

/*************************************HELPER FUNCTIONS***************************************/
/**
 * Function to get the output value of a given neuron.
 * Used primarily by the interfacing functions to show
 * the user network information and the UI
 */
bool outputBool(uint16_t cellID) {
  return outputList[cellID];
}

/**
 * A small function to draw out the muscle ratio information to the screen.
 * Used by the UI to make information more readable for the user.
 */
void printMovementDir(uint16_t xpos, uint16_t ypos) {
  if (vaRatio + daRatio > vbRatio + dbRatio) {
    arduboy.setCursor(xpos, ypos);
    arduboy.print("BACKWARD-");
  } else {
    arduboy.setCursor(xpos, ypos);      
    arduboy.print("FORWARD-");
  }
  
  if (vaRatio + vbRatio > daRatio + dbRatio) {
    arduboy.print("L/VENTRAL");
  } else {
    arduboy.print("R/DORSAL");
  }
}

/*************************NOTES*****************************/

/**
 *                         References
 * http://ims.dse.ibaraki.ac.jp/ccep-tool/
 * https://www.sciencedirect.com/science/article/pii/S0960982205009401
 * https://pubmed.ncbi.nlm.nih.gov/40666838/
 * https://www.nature.com/articles/s42003-021-02561-9
 * https://www.wormatlas.org/hermaphrodite/nervous/Neuroframeset.html
 * https://www.wormatlas.org/neurons/Individual%20Neurons/ASIframeset.html
 * https://www.ncbi.nlm.nih.gov/books/NBK19787/
 * https://www.science.org/doi/10.1126/science.aam6851
 * https://www.cell.com/current-biology/fulltext/S0960-9822(14)01501-2
 */

 //escape behavior - fear (O_O)
 //slow reversal - indecisive (-_n)
 //foraging behavior - happy (^v^)
 //attractive chemosensation - content (^-^)
 //repulsive chemosensation - disgust? (-x-)
 //noxious temp, harsh touch, bad air - pain/discomfort (>.<)
 //temperature shock response - (@_@)
 //lethargy, social sleeping - sleepy (UwU)
 //social feeding - ???
 //solitary feeding - ???
 //egg laying - ???

/**
 *                     SENSORY INFO
 *
 * Gentle Nose Touch (suppress lateral foraging, head withdrawal, escape behavior)- OLQ, IL1, FLP
 * Gentle Anterior Body Touch (causes backwards movement) - ALM, AVM
 * Gentle Posterior Body Touch (causes forward movement) - PLM
 * Harsh Body Touch (causes reversal) - PVD, potentially also ALM
 * Harsh Head/Nose Touch - FLP, ASH, potentially also OLQ
 * Osmomtic Pressure - potentially ASH and ADL
 * Texture Sensation (sense small round objects, food-induced slowing foraging behavior) - CEP, ADE, PDE
 * Proprioception (stretch and tension during movements) - DVA, potentially PHC and PVD
 * Nociception - ASH
 * Attractive Chemosensation - ADF, ASE (primary sensor), ASG, ASI, ASJ, ASK
 * Repulsive Chemosensation - ASH (primary nociceptor), ADL, ASK, ASE
 * ??? chemo - AWA, AWB, AWB
 * Thermosensation - AFD (primary sensor - seeks cooler temp), AWC (seeks warmer temp), ASI
 * Noxious Cold Thermosensation - PVD (acute cold-shock)
 * Noxious Heat Thermosensation (prolonged 30*C causes heat-shock response/thermal avoidance) - AFD (head), FLP (head), and PHC (tail)
 * Photosensation (photoboic movement reversal response) - potentially VNC but largely unknown
 * Oxygen sensation (nociceptive) - AQR, PQR, URX, possibly SDQ, ALN, BDU, ADF, and ASH
 * Carbon Dioxide sensation (nociceptive, particularly when well-fed) - FD, BAG, and AE
 */
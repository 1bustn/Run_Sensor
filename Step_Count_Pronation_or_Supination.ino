//void setup() {
  // put your setup code here, to run once:

}

//void loop() {
  // put your main code here, to run repeatedly:
  //3 variables for piezo voltage
  //3 variables for supination count, pronation count, and neutral count
  //boolean for if step is recorded
  //variable for step count
  //timer for a 75ms period to record the first step to step count and peak voltages
  //225ms timer for after the 75ms period to not receive any readings from the piezos
  //while arduino is on
    //when any piezo sensor is triggered, turn variable state to 1
      //add one to step count
      //start 75ms timer
    //the peak readings for each piezo is recorded to variable
    //code to compare the peak voltage readings
      //add one to the category counter based on the readings
      //reset the step boolean
      //225ms timer to ignore other inputs from piezo
  

}

/*****************************************************************
 * File Name:     FSAEDAQ2014_GPS_LOGGING.ino
 * Written by:    Aman Azad
 * Description:   Use interupts to recieve GPS NMEA Sentences
                  once per second, parse sentences (comma 
                  delimited), then log into SD card.
*****************************************************************/

/* Function Header Format */
/*****************************************************************
 * Function:     
 * Description:
 * Parameters: 
*****************************************************************/
/* General library for GPS related functions */
#include <Adafruit_GPS.h>

/* Communicate with GPS using software serial input */
#ifdef __AVR__
  #include <SoftwareSerial.h>
  #include <avr/sleep.h>
#endif

/* Libraries for SD logging */
#include <SPI.h>
#include <SD.h>

#ifdef __AVR__
SoftwareSerial mySerial(8, 7); /* GPS is connected on the shield
                                  to these pins */
#else
#define mySerial Serial1
#endif

Adafruit_GPS GPS(&mySerial); /* assign address of mySerial to new
                                Adafruit_GPS object */

/* Set GPSECHO to 'false' to tourn off echoing the GPS data to the
   Serial Console */
#define GPSECHO false

/* Set to true only, only log to SD if GPS has a fix, for debugging,
   keep false */
#define LOG_FIXONLY false

/* Set the pins used */
#define chipSelect 10

File logfile;

/*****************************************************************
 * Function: parseHex
 * Description: Convert a passed in hexadecimal value into decimal
 * Parameters: hexadecimal char c to be conveted to decimal 
 *             equivalient
*****************************************************************/
uint8_t parseHex(char c)
{
  if (c < '0')
  {
    return 0;
  }
  if (c <= '9')
  {
    return c - '0'; /* subtracting ASCII 0 returns decimal equivalent,
                       lookup ASCII table to see why this works */
  }
  if (c < 'A')
  {
    return 0;
  }
  if (c <= 'F')
  {
    return (c - 'A')+10; /* Characters A thru F are 10 thru 16 */
  }
}

/*****************************************************************
 * Function:     error
 * Description:  Print out error codes to the Serial console if 
 *               error in SD card
 * Parameters:   None
*****************************************************************/
void error()
{
  if (SD.errorCode())
  {
    putstring("SD error: ");
    Serial.print(card.errorCode(), HEX);
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
  }
}

/*****************************************************************
 * Function:     setup
 * Description:  Instantiates necessary pins and objects for GPS
 * Parameters:   None.
*****************************************************************/
void setup()
{
  Serial.begin(115200); /* select fastest baud rate to not drop
                           characters from GPS sentences */
  Serial.println("\r\nFSAE DAQ GPS Logger");
  
  pinMode(10, OUTPUT); /* set default chip select to output 
                          regardless if used or not */
                          
  /* Check if SD Card is present and can be initialized */
  if (!SD.begin(chipSelect))
  {
    Serial.println("Card initializtion failed!"); /* Catastrophic
                                                     failure */
    
    error(); /* Push errors to Serial console */
  }
  
  char filename[15]; /* string used to hold name of file log data
                        is being sent to */
  strcpy(filename, "GPSLOG00.TXT");
  for (uint8_t index = 0; index < 100; i++)
  {
    /* update new log file names to avoid overwrites */
    filename[6] = '0' + index/10;
    filename[7] = '0' + index%10;
    
    /* create if does not exist */
    if (!SD.exists(filename))
    {
      break;
    }
  }
  
  logfile = SD.open(filename, FILE_WRITE);
  if (!logfile) /* log file unable to be created!! */
  {
    Serial.print("Couldn't create "); Serial.println(filename);
    error();
  }
  Serial.print("Writing to: "); Serial.println(filename);
  
  GPS.begin(9600); /* Set GPS to baud rate of 9600;
                      note this is different from the
                      Serial baud rate */
  /* Uncomment this line to turn on RMC (recommended minimum) and
     GGA (fix data) including altitude */
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  /* Uncomment this line to turn on only the minimum recommended data */
  /* GPS.sendCommand(PTK_SET_NMEA_OUTPUT_RMCONLY); */
  /* For logging, best to keep at either these settings to prevent file
     size overflow */
  
  /* Set the update rate */
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); /* 1 or 5hz update rates */
  
  /* Turn off updates on antenna status, if the firmware permits it */
  GPS.sendCommand(PGCMD_NOANTENNA);
  
  Serial.println("Ready!");
}

/*****************************************************************
 * Function:     loop
 * Description:  Main loop program
 * Parameters:   None
*****************************************************************/
void loop()
{
  char c = GPS.read();
  
  if (GPSECHO)
  {
    if (c)
    {
      Serial.print(c); /* print GPS data to Serial console */
    }
  }
  
  /* If a sentence recieved, check the checksum and parse it */
  if (GPS.newNMEArecieved())
  {
    /* printing out NMEA sentence prevents reading new data! */
    /* Serial.println(GPS.lastNMEA()); */ /* This sets
                                    newNMEArecieved to false */
                                    
    if (!GPS.parse(GPS.lastNMEA())) /*also sets newNMEArecieved to false */
    {
      return; /* if we fail to parse a sentence, just wait for another 
                 sentence */
    }
    
    /* Sentenced sucessfully parsed! */
    Serial.println("OK");
    if (LOG_FIXONLY && !GPS.fix)
    {
      Serial.print("No Fix");
      return; /* skip this sentence, wait for another */
    }
    
    /* Proceed with logging */
    Serial.println("LOGGING");
    
    char *stringptr = GPS.lastNMEA();  /* hold value of sentence */
    uint8_t stringsize = strlen(stringptr); /* hold value of length
                                               of sentence */
    if (stringsize != logfile.write((uint8_t *)stringptr, stringsize))
    {
      error(); /* could not print to SD card */
    }
    if (strstr(stringptr, "RMC"))
    {
      /* RMC found in string (passed in string pointer) */
      logfile.flush(); /* save to logfile */
    }
    Serial.println();
  }
}

/* EOF */

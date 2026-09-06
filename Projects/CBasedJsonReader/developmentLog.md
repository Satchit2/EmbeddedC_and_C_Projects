# Custom C based .json file reader (NO AI)

## ACTIVE VERSION:  None
## DEV VERSION: v0.1

### Dev Version Goals:
#### Goals for the v0.1 development include
- Be able to read through a simple .json file without nested object.
- Create simple functions like edit and delete attributes of the object.
- Perform testing to ensure encapsulation of the objects and functions used in the implementation file.
- Add basic error handling

## Development Logs:

### *4 August:*

##### Version: v0.1 (in-progress)
##### Description: 
Created basic placeholder functions and opaque object in the header file with its implementation present in the .c implement file. Yet to fully implement all the said things

### *5 August:*

##### Version: v0.1 (in-progress)
##### Description: 
Furthered the basic structure of the header and implementation file. Added basic error handling type and functions. Building implementation file functions using single exit structure.
Created the Initializer for the json object. Currrently working on compatibility with the json format similar to trial.json in Data folder. 

### *9 August:*

##### Version: v0.1 (in-progress)
##### Description: 
Planned the structure to perform reading and dry ran SIMD format calculations along with learning SIMD commands. Implemented a custom compat checking function to make sure that device running the library can use SIMD functions.

### *7 September:*

##### Version: v0.1 (in-progress)
##### Description: 
Created required SIMD Function compile time compat check. Also developed certain required helper functions and developed JSON to C struct parser for simple JSON files without arrays or nested objects. Planned testcases but yet to run them.
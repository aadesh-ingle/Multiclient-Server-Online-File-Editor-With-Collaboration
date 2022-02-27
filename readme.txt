compile using :
		gcc client.c -o client  //for client
		gcc server.c -o server	//for server
For "insert" command:
		if the first double quote(") is typed then the command will not end till the second double quote is typed.
		you can even type after second double quote but the parser will treat it as wrong command as mentioned in assignment
		anything inside of double quotes will be treated as a message
For "invite" command:
		the client that is being invited will have to type only "yes" or "Yes"  or "No" or "no" considering the cases also.Anything else will be treated as invalid command                 and will not be sent to server and will ask the user to enter again in loop.
		The permission should be mentioned as capital letters - "E"  or "V".
		Small case permission letter will not be accepted e.g. - "e" or "v".
For "read" command: 
		if file is empty. it returns that "the line no. 'x' is not present" instead of  "the file is empty" according to design choice.

permfile.txt saves the details of the files that are on the server side.

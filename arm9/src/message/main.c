void death(char *message, u8 *buffer){
	iprintf("%s\n", message);
	iprintf("Hold Power to exit\n");
	free(buffer);
	while(1)swiWaitForVBlank();
}
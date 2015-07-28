/*
rv=select(sockdup+1, &set, NULL, NULL, &timeout);
if(rv == -1)
{
  perror("Select"); // Error
  break;
}
else
  if(rv == 0)
  {
    perror("Timeout"); // Timeout
    printf("Conexión con %s finalizada",inet_ntoa(their_addr.sin_addr) );
    close(sockdup);
    exit(1);
   }
  else
  {  //Data to read
     if((numbytes=  recv(sockdup,bufTCP,sizeof(bufTCP),0))== -1)
     {
        perror("Error de lectura en el socket");
        printf("Conexión con %s finalizada",inet_ntoa(their_addr.sin_addr) );
        close(sockdup);
        exit(1);
      }
     else
        {
          printf("%s\n",bufTCP);
          //FD_ZERO(&set);
          sleep(5);
        }
    }

    */

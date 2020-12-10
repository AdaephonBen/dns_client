/* A simple DNS client to talk to Google's DNS Server(8.8.8.8)
 * and resolve hostnames to IPv4 addresses. 
 * Written by Vishnu VS
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

char* get_qname(char* url) {
    int i;
    char *qname;
    qname = (char *) malloc (strlen(url)+3);
    char *temp;
    temp = (char *)malloc(strlen(url)+2);
    int currentLength = 0;
    int currentPointer = 0;

    for (i = 0 ; i < strlen(url) ; i++) {
        if (url[i] == '.') {
            qname[currentPointer] = currentLength + '0' ;
            currentPointer++;

            for (int j = 0 ; j < strlen(temp) ; j++, currentPointer++) {
                qname[currentPointer] = temp[j];
            }
            memset(temp, 0, strlen(temp));
            currentLength = 0;
        } else {
            temp[currentLength] = url[i];
            currentLength++;
        }
    }

    qname[currentPointer] = currentLength + '0';
    currentPointer++;
    for (int j = 0 ; j < strlen(temp) ; j++, currentPointer++) {
        qname[currentPointer] = temp[j];
    }
    qname[currentPointer] = '0';
    return qname;
}


int main(int argc, char *argv[]) {

    if (argc != 2) { // Check for argument
        printf("No hostname specified. Exiting...\n");
        return 1;
    } 

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0); // Didn't know which domain to use so used AF_INET = IPv4

    if (socket_fd == -1) {
        printf("Unable to open socket...\n");
        printf("%s", strerror(errno));
        return 1;
    }
    struct sockaddr_in google_address;

    google_address.sin_family = AF_INET;
    google_address.sin_port = htons(53); // DNS Port is UDP 53
    
    int errorPton = inet_pton(AF_INET, "8.8.8.8", &google_address.sin_addr);
    if (errorPton < 0) {
        printf("Unable to convert from text to binary");
        return 1;
    }

    unsigned int dns_req_id = 0xe808;
    unsigned int dns_req_flags = 0x0100;
    unsigned int dns_req_questions = 0x0001;
    unsigned int dns_req_answers, dns_req_authorities, dns_req_additionals;
    dns_req_additionals = dns_req_authorities = dns_req_answers = 0x0000;

    char* qname = get_qname(argv[1]); 
    unsigned int dns_question_qtype = 0x0001;
    unsigned int dns_question_qclass = 0x0001;

    unsigned int dns_header_array[] = {dns_req_id, dns_req_flags, dns_req_questions, dns_req_answers, dns_req_authorities, dns_req_additionals};
    int l = sizeof(dns_header_array)/sizeof(unsigned int);
    unsigned char dns_header[2*l];
    for (int i = 0 ; i < l ; i++) {
        dns_header[2*i] = dns_header_array[i] / 256 ;
        dns_header[2*i+1] = dns_header_array[i] % 256 ;
    }
    int lengthDNS = 2*l + strlen(qname) + 4; 
    unsigned char dns_req[lengthDNS];
    for (int i = 0 ; i < 2*l ; i++) {
        dns_req[i] = dns_header[i];
    }
    for (int i = 0 ; i < strlen(qname) ; i++) {
        if (qname[i] >= 48 && qname[i] <= 57) {
            dns_req[2*l+i] = (qname[i]-'0');
        } else {
            dns_req[2*l+i] = (qname[i]);
        }
    }
    dns_req[lengthDNS-4] = dns_question_qtype / 256;
    dns_req[lengthDNS-3] = dns_question_qtype % 256;
    dns_req[lengthDNS-2] = dns_question_qclass / 256;
    dns_req[lengthDNS-1] = dns_question_qclass % 256;
    
    if (sendto(socket_fd, dns_req, sizeof(dns_req), 0, (struct sockaddr *) &google_address, sizeof(google_address)) != sizeof(dns_req)) {
        printf("sendto error \n") ;
    }

    socklen_t length = 0;
    uint8_t response[512];
    memset (&response, 0, 512);

    ssize_t bytes = recvfrom (socket_fd, response, 512, 0, (struct sockaddr *) &google_address, (socklen_t *)sizeof(google_address));

    unsigned char ip[4];
    for (int i = lengthDNS+12 ; i < lengthDNS+16 ; i++) {
        ip[i-lengthDNS-12] = response[i];
    }
    printf("IPv4 Address: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

    return 0;
}

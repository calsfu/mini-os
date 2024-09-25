#include "tls.h"
#define MAX_THREADS 128

int initialized = 0;
unsigned int page_size; //dependent on system

typedef struct thread_local_storage
{
    pthread_t tid;
    unsigned int size; /* size in bytes */
    unsigned int page_num; /* number of pages */
    struct page **pages; /* array of pointers to pages */
} TLS;


struct page {
    unsigned long address; /* start address of page */
    int ref_count; /* counter for shared pages */
};

typedef struct node
{
    pthread_t tid;
    TLS *tls;
    struct node *next;
    struct node *prev;
} Node;

int clone = 0; 

Node* head = NULL; 
Node* tail = NULL;
// TLS* hash_table[MAX_THREADS]; 
Node* check_valid_tls(pthread_t tid) {
    Node* curr = head;
    while(curr != NULL) {
        if(curr->tid == tid) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void tls_handle_page_fault(int sig, siginfo_t *si, void *context)
{
    TLS *tls = check_valid_tls(pthread_self())->tls;
    unsigned long p_fault = ((unsigned long) si->si_addr) & ~(page_size - 1);
    int i;
    for(i = 0;i < tls->page_num;i++) {
        if(tls->pages[i]->address == p_fault) {
            pthread_exit(NULL);
        }
    }
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    raise(sig);
}



void tls_init()
{
    struct sigaction sigact;
    /* get the size of a page */
    page_size = getpagesize();
    /* install the signal handler for page faults (SIGSEGV, SIGBUS) */
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO; /* use extended signal handling */
    sigact.sa_sigaction = tls_handle_page_fault;
    sigaction(SIGBUS, &sigact, NULL);
    sigaction(SIGSEGV, &sigact, NULL);
    initialized = 1;
    head = NULL;
    tail = NULL;
    // for(i = 0;i < MAX_THREADS;i++) {
    //     hash_table[i] = NULL;
    // }
}

void tls_protect(struct page *p)
{
    if (mprotect((void *) p->address, page_size, 0)) {
        fprintf(stderr, "tls_protect: could not protect page\n");
        exit(1);
    }
}

void tls_unprotect(struct page *p)
{
    if (mprotect((void *) p->address, page_size, PROT_READ | PROT_WRITE)) {
        fprintf(stderr, "tls_unprotect: could not unprotect page\n");
        exit(1);
    }
}

int tls_create(unsigned int size) {
    //error handling
    if(!initialized) {
        tls_init();
    }
    if(size <= 0) {
        return -1;
    }
    if(check_valid_tls(pthread_self()) != NULL) {
        return -1;
    }
    //intalize signal handler if not already
    
    //allocate and intalize tls
    unsigned int page_num = (size / page_size) + 1;
    TLS* tls = calloc(1, sizeof(TLS));
    tls->tid = pthread_self();
    tls->size = size;
    tls->page_num = page_num;
    //allocate and intalize pages
    tls->pages = calloc(tls->page_num, sizeof(struct page*) );
    int i;
    if(!clone) {
        //only need to allocate pages if clone is not called
        for(i = 0; i < tls -> page_num;i++) {
            struct page *p = calloc(1, sizeof(struct page));
            p->address = (unsigned long)mmap(0, page_size, 0, MAP_ANON | MAP_PRIVATE, 0, 0);
            p->ref_count = 1;
            tls->pages[i] = p;
        }
    }
    clone = 0;
    //add to  global hash
    // struct hash_element *element = calloc(1, sizeof(struct hash_element));
    // element->tid = tls->tid;
    // element->tls = tls;
    // element->next = NULL;
    //create a new node and add to end of list
    Node* node = calloc(1, sizeof(Node));
    node->tid = tls->tid;
    node->tls = tls;
    node->next = NULL;
    node->prev = tail;
    if(head == NULL) {
        head = node;
    }
    else {
        tail->next = node;
        node->prev = tail;
    }
    tail = node;
    //add tls to global hash table
    // if(hash_table[index] == NULL) {
    // hash_table[index] = tls;
    // } 
    // else {
    //     struct hash_element *temp = hash_table[index];
    //     while(temp->next != NULL) {
    //         temp = temp->next;
    //     }
    //     temp->next = element;
    // }
    return 0;
}

int tls_write(unsigned int offset, unsigned int length, char *buffer) {
    //error handling
    if(!initialized) {
        return -1;
    }
    Node* curr = check_valid_tls(pthread_self());
    if(curr == NULL) {
        return -1;
    }
    TLS* tls = curr->tls;
    if(offset + length > tls->size) {
        return -1;
    }
    if(buffer == NULL) {
        return -1;
    }
    int i;
    //unprotect pages
    for(i = 0;i < tls->page_num;i++) {
        tls_unprotect(tls->pages[i]);
    }
    //write pages
    int cnt, idx;
    for (cnt = 0, idx = offset; idx < (offset + length); ++cnt, ++idx) {
        struct page *p;
        unsigned int pn, poff;
        pn = idx / page_size;
        poff = idx % page_size;
        p = tls->pages[pn];
        if (p->ref_count > 1) {
            //CoW implementation
            //create new pages to write to
            struct page *copy;
            copy = (struct page *) calloc(1, sizeof(struct page));
            copy->address = (unsigned long) mmap(0,
            page_size, PROT_WRITE,
            MAP_ANON | MAP_PRIVATE, 0, 0);
            copy->ref_count = 1;
            //copy contents of original page to new page
            memcpy((void *) copy->address, (void *) p->address, page_size);
            tls->pages[pn] = copy;
            /* update original page */
            p->ref_count--;
            tls_protect(p); //Gets protected after
            p = copy;
        }
        char* dst = ((char *) p->address) + poff;
        *dst = buffer[cnt];
    }
    //protect pages
    for(i = 0;i < tls->page_num;i++) {
        tls_protect(tls->pages[i]);
    }
    return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer) {
    if(!initialized) {
        return -1;
    }
    Node* curr = check_valid_tls(pthread_self());
    if(curr == NULL) {
        return -1;
    }
    TLS* tls = curr->tls;
    if(offset + length > tls->size) {
        return -1;
    }
    int i;
    for(i = 0;i < tls->page_num;i++) {
        tls_unprotect(tls->pages[i]);
    }
    //read pages
    int cnt, idx;
    for (cnt = 0, idx = offset; idx < (offset + length); ++cnt, ++idx) {
        struct page *p;
        unsigned int pn, poff;
        pn = idx / page_size;
        poff = idx % page_size;
        p = tls->pages[pn];
        char* src = ((char *) p->address) + poff;
        buffer[cnt] = *src;
    }
    for(i = 0;i < tls->page_num;i++) {
        tls_protect(tls->pages[i]);
    }
    return 0;
}

int tls_destroy() {
    if(!initialized) {
        return -1;
    }
    Node* temp = check_valid_tls(pthread_self());
    if(temp == NULL) {
        return -1;
    }
    TLS* tls = temp->tls;
    int i = 0;
    for(i = 0;i < tls->page_num;i++) {
        if(tls->pages[i]->ref_count > 1) {
            tls->pages[i]->ref_count--;
        } else {
            munmap((void*) tls->pages[i]->address, page_size);
        }
    }
    Node* curr = check_valid_tls(pthread_self());
    if(curr -> prev != NULL) {
        curr -> prev -> next = curr -> next;
    }
    if(curr -> next != NULL) {
        curr -> next -> prev = curr -> prev;
    }
    if(curr == head) {
        head = curr -> next;
    }
    if(curr == tail) {
        tail = curr -> prev;
    }
    free(tls->pages);
    free(curr);
    free(tls);
    
    return 0;
}

int tls_clone(pthread_t tid) {
    if(!initialized) {
        return -1;
    }
    if(tid == pthread_self()) {
        return -1;
    }
    Node* old_node = check_valid_tls(tid);
    Node* new_node = check_valid_tls(pthread_self());    

    if(old_node == NULL) {
        return -1;
    }
    if(new_node != NULL) {
        return -1;
    }
    TLS* old_tls = old_node->tls;
    unsigned int size = old_tls->size;
    clone = 1;
    tls_create(size);
    if(check_valid_tls(pthread_self()) == NULL) {
        return -1;
    }
    TLS* new_tls = check_valid_tls(pthread_self())->tls;
    int i;
    //copy pages
    for(i = 0;i < old_tls->page_num;i++) {
        old_tls->pages[i]-> ref_count++;
        new_tls->pages[i] = old_tls->pages[i];

    }
    return 0;
    //finish
}

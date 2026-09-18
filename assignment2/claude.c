/* ============================================================
 * email_priority.c
 *
 * Prioritizes a CEO's inbox using a MaxHeap-based priority queue.
 *
 * The MaxHeap is implemented from scratch as a dynamically-growing
 * array (a "list-based" heap, as opposed to a pointer/tree-based
 * heap). No standard-library or third-party heap module is used --
 * every heap operation below (insert, extract-max, sift up/down)
 * is written by hand.
 *
 * -------------------------------------------------------------
 * Priority rules
 * -------------------------------------------------------------
 * 1. Sender category determines the primary priority:
 *        Boss            (highest / read first)
 *        Subordinate
 *        Peer
 *        ImportantPerson
 *        OtherPerson     (lowest / read last)
 *
 * 2. When two queued emails share the same category, the NEWER
 *    email (by date) is read first.
 *
 * Both rules are folded into a single numeric key so a standard
 * MaxHeap comparison (bigger key = higher priority) handles both
 * at once:
 *
 *        key = categoryRank * 100000000  +  YYYYMMDD
 *
 * Category rank dominates the comparison (it's multiplied by a
 * number bigger than any possible YYYYMMDD value), so category
 * always wins first; the date only breaks ties within the same
 * category.
 *
 * -------------------------------------------------------------
 * Commands (one per line of the test file)
 * -------------------------------------------------------------
 *   EMAIL <sender category>,<subject line>,<date>
 *          -> insert a new email into the heap#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_SENDER_LEN   32
#define MAX_SUBJECT_LEN  256
#define MAX_DATE_LEN     16
#define MAX_LINE_LEN     1024
#define INITIAL_CAPACITY 16

/* ------------------------------------------------------------
 * Email record stored in the heap.
 * ------------------------------------------------------------ */
typedef struct {
    char sender[MAX_SENDER_LEN];    /* sender category, e.g. "Boss"   */
    char subject[MAX_SUBJECT_LEN];  /* subject line (no commas)       */
    char date[MAX_DATE_LEN];        /* MM-DD-YYYY, kept as given       */
    long priorityKey;               /* combined category+date sort key */
} Email;

/* ------------------------------------------------------------
 * MaxHeap: a growable array of Emails plus a logical size.
 * ------------------------------------------------------------ */
typedef struct {
    Email *data;
    int size;
    int capacity;
} MaxHeap;

/* ============================================================
 * Heap helper / core functions -- all written from scratch.
 * ============================================================ */

/* Allocate the heap's backing array. */
void heapInit(MaxHeap *h) {
    h->capacity = INITIAL_CAPACITY;
    h->size = 0;
    h->data = (Email *)malloc(sizeof(Email) * (size_t)h->capacity);
    if (h->data == NULL) {
        fprintf(stderr, "Fatal: memory allocation failed in heapInit.\n");
        exit(1);
    }
}

/* Release everything the heap owns. */
void heapFree(MaxHeap *h) {
    free(h->data);
    h->data = NULL;
    h->size = 0;
    h->capacity = 0;
}

/* Double the backing array's capacity when it fills up. */
void heapGrow(MaxHeap *h) {
    int newCapacity = h->capacity * 2;
    Email *newData = (Email *)realloc(h->data, sizeof(Email) * (size_t)newCapacity);
    if (newData == NULL) {
        fprintf(stderr, "Fatal: memory allocation failed in heapGrow.\n");
        exit(1);
    }
    h->data = newData;
    h->capacity = newCapacity;
}

void swapEmails(Email *a, Email *b) {
    Email temp = *a;
    *a = *b;
    *b = temp;
}

/* Restore heap order by moving the element at 'index' upward
 * toward the root while it is bigger than its parent. */
void siftUp(MaxHeap *h, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (h->data[index].priorityKey > h->data[parent].priorityKey) {
            swapEmails(&h->data[index], &h->data[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

/* Restore heap order by moving the element at 'index' downward,
 * always swapping with the larger of its two children, until
 * both children (if any) are smaller or it has none. */
void siftDown(MaxHeap *h, int index) {
    while (1) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int largest = index;

        if (left < h->size && h->data[left].priorityKey > h->data[largest].priorityKey) {
            largest = left;
        }
        if (right < h->size && h->data[right].priorityKey > h->data[largest].priorityKey) {
            largest = right;
        }
        if (largest == index) {
            break;
        }
        swapEmails(&h->data[index], &h->data[largest]);
        index = largest;
    }
}

/* Insert a new email into the heap, keeping heap order. */
void heapInsert(MaxHeap *h, Email e) {
    if (h->size == h->capacity) {
        heapGrow(h);
    }
    h->data[h->size] = e;
    siftUp(h, h->size);
    h->size++;
}

/* Remove and return the highest-priority email.
 * Returns 1 and fills *out on success, 0 if the heap is empty. */
int heapExtractMax(MaxHeap *h, Email *out) {
    if (h->size == 0) {
        return 0;
    }
    *out = h->data[0];
    h->size--;
    h->data[0] = h->data[h->size];
    siftDown(h, 0);
    return 1;
}

/* Look at the highest-priority email without removing it.
 * Returns 1 and fills *out on success, 0 if the heap is empty. */
int heapPeek(MaxHeap *h, Email *out) {
    if (h->size == 0) {
        return 0;
    }
    *out = h->data[0];
    return 1;
}

/* ============================================================
 * Parsing helpers
 * ============================================================ */

/* Trim leading/trailing whitespace from a string, in place. */
void trim(char *str) {
    int start = 0;
    while (isspace((unsigned char)str[start])) {
        start++;
    }
    if (start > 0) {
        memmove(str, str + start, strlen(str) - (size_t)start + 1);
    }
    int len = (int)strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

/* Map a sender category string to its priority rank.
 * Higher number = higher priority = read sooner. */
int categoryRank(const char *category) {
    if (strcmp(category, "Boss") == 0)            return 5;
    if (strcmp(category, "Subordinate") == 0)      return 4;
    if (strcmp(category, "Peer") == 0)             return 3;
    if (strcmp(category, "ImportantPerson") == 0)  return 2;
    if (strcmp(category, "OtherPerson") == 0)      return 1;
    return 0; /* unrecognized category: treated as lowest priority */
}

/* Convert MM-DD-YYYY into a comparable integer YYYYMMDD.
 * Bigger result = later (more recent) date. */
long dateToInt(const char *date) {
    int mm = 0, dd = 0, yyyy = 0;
    if (sscanf(date, "%d-%d-%d", &mm, &dd, &yyyy) != 3) {
        return 0; /* malformed date sorts as oldest possible */
    }
    return (long)yyyy * 10000L + (long)mm * 100L + (long)dd;
}

/* Parse a line of the form:
 *     EMAIL <sender category>,<subject line>,<date>
 * (commas are the field delimiters; the subject line itself is
 * guaranteed not to contain commas) and insert it into the heap. */
void processEmailCommand(MaxHeap *h, char *line) {
    /* Skip the leading "EMAIL " token. */
    char *rest = line + 6;

    char *senderTok  = strtok(rest, ",");
    char *subjectTok = strtok(NULL, ",");
    char *dateTok    = strtok(NULL, ",");

    if (senderTok == NULL || subjectTok == NULL || dateTok == NULL) {
        fprintf(stderr, "Skipping malformed EMAIL line: %s\n", line);
        return;
    }

    Email e;
    strncpy(e.sender, senderTok, MAX_SENDER_LEN - 1);
    e.sender[MAX_SENDER_LEN - 1] = '\0';
    trim(e.sender);

    strncpy(e.subject, subjectTok, MAX_SUBJECT_LEN - 1);
    e.subject[MAX_SUBJECT_LEN - 1] = '\0';
    trim(e.subject);

    strncpy(e.date, dateTok, MAX_DATE_LEN - 1);
    e.date[MAX_DATE_LEN - 1] = '\0';
    trim(e.date);

    int rank = categoryRank(e.sender);
    long dateVal = dateToInt(e.date);
    e.priorityKey = (long)rank * 100000000L + dateVal;

    heapInsert(h, e);
}

/* ============================================================
 * Command handlers
 * ============================================================ */

void handleNext(MaxHeap *h) {
    Email e;
    if (heapPeek(h, &e)) {
        printf("Next email:\n");
        printf("\tSender: %s\n", e.sender);
        printf("\tSubject: %s\n", e.subject);
        printf("\tDate: %s\n", e.date);
        printf("\n");
    } else {
        printf("No emails to read.\n\n");
    }
}

void handleRead(MaxHeap *h) {
    Email e;
    /* Silently discard the top email. If the heap is empty,
     * heapExtractMax simply returns 0 and nothing happens --
     * no crash, no output, matching the "handle gracefully"
     * requirement. */
    heapExtractMax(h, &e);
}

void handleCount(MaxHeap *h) {
    if (h->size == 1) {
        printf("There is 1 email to read.\n\n");
    } else {
        printf("There are %d emails to read.\n\n", h->size);
    }
}

/* ============================================================
 * Main: read commands one line at a time from a file named on
 * the command line, or from stdin if no filename is given.
 * ============================================================ */
int main(int argc, char *argv[]) {
    FILE *input = stdin;

    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            fprintf(stderr, "Could not open file: %s\n", argv[1]);
            return 1;
        }
    }

    MaxHeap heap;
    heapInit(&heap);

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), input) != NULL) {
        /* Strip trailing newline / carriage return. */
        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) == 0) {
            continue; /* ignore blank lines */
        }

        if (strncmp(line, "EMAIL ", 6) == 0) {
            processEmailCommand(&heap, line);
        } else if (strcmp(line, "NEXT") == 0) {
            handleNext(&heap);
        } else if (strcmp(line, "READ") == 0) {
            handleRead(&heap);
        } else if (strcmp(line, "COUNT") == 0) {
            handleCount(&heap);
        } else {
            fprintf(stderr, "Skipping unrecognized command: %s\n", line);
        }
    }

    if (input != stdin) {
        fclose(input);
    }
    heapFree(&heap);
    return 0;
}
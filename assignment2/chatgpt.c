

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 2048
#define SENDER_SIZE 32
#define SUBJECT_SIZE 1024
#define DATE_SIZE 11

typedef struct {
    char sender[SENDER_SIZE];
    char subject[SUBJECT_SIZE];
    char date[DATE_SIZE];
    int sender_priority;
    int date_priority;
    unsigned long arrival_order;
} Email;

/* A max heap stored in a dynamically growing list (array). */
typedef struct {
    Email *list;
    size_t size;
    size_t capacity;
} MaxHeap;

static void initializeHeap(MaxHeap *heap)
{
    heap->list = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

static void destroyHeap(MaxHeap *heap)
{
    free(heap->list);
    heap->list = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

static int getSenderPriority(const char *sender)
{
    if (strcmp(sender, "Boss") == 0) {
        return 5;
    }
    if (strcmp(sender, "Subordinate") == 0) {
        return 4;
    }
    if (strcmp(sender, "Peer") == 0) {
        return 3;
    }
    if (strcmp(sender, "ImportantPerson") == 0) {
        return 2;
    }
    if (strcmp(sender, "OtherPerson") == 0) {
        return 1;
    }
    return 0;
}

/* Converts MM-DD-YYYY to YYYYMMDD so newer dates have larger values. */
static int getDatePriority(const char *date)
{
    int month;
    int day;
    int year;

    if (sscanf(date, "%d-%d-%d", &month, &day, &year) != 3) {
        return 0;
    }

    return year * 10000 + month * 100 + day;
}

/* Returns nonzero when a has greater priority than b. */
static int hasHigherPriority(const Email *a, const Email *b)
{
    if (a->sender_priority != b->sender_priority) {
        return a->sender_priority > b->sender_priority;
    }
    if (a->date_priority != b->date_priority) {
        return a->date_priority > b->date_priority;
    }

    /* If sender category and date tie, preserve file order. */
    return a->arrival_order < b->arrival_order;
}

static void swapEmails(Email *a, Email *b)
{
    Email temporary = *a;
    *a = *b;
    *b = temporary;
}

static int growHeap(MaxHeap *heap)
{
    size_t new_capacity = (heap->capacity == 0) ? 8 : heap->capacity * 2;
    Email *new_list = realloc(heap->list, new_capacity * sizeof(Email));

    if (new_list == NULL) {
        return 0;
    }

    heap->list = new_list;
    heap->capacity = new_capacity;
    return 1;
}

static void heapifyUp(MaxHeap *heap, size_t index)
{
    while (index > 0) {
        size_t parent = (index - 1) / 2;

        if (!hasHigherPriority(&heap->list[index], &heap->list[parent])) {
            break;
        }

        swapEmails(&heap->list[index], &heap->list[parent]);
        index = parent;
    }
}

static void heapifyDown(MaxHeap *heap, size_t index)
{
    while (1) {
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;
        size_t largest = index;

        if (left < heap->size &&
            hasHigherPriority(&heap->list[left], &heap->list[largest])) {
            largest = left;
        }
        if (right < heap->size &&
            hasHigherPriority(&heap->list[right], &heap->list[largest])) {
            largest = right;
        }
        if (largest == index) {
            break;
        }

        swapEmails(&heap->list[index], &heap->list[largest]);
        index = largest;
    }
}

static int insertEmail(MaxHeap *heap, Email email)
{
    if (heap->size == heap->capacity && !growHeap(heap)) {
        return 0;
    }

    heap->list[heap->size] = email;
    heapifyUp(heap, heap->size);
    heap->size++;
    return 1;
}

static const Email *peekMax(const MaxHeap *heap)
{
    if (heap->size == 0) {
        return NULL;
    }
    return &heap->list[0];
}

static int removeMax(MaxHeap *heap)
{
    if (heap->size == 0) {
        return 0;
    }

    heap->size--;
    if (heap->size > 0) {
        heap->list[0] = heap->list[heap->size];
        heapifyDown(heap, 0);
    }
    return 1;
}

static void removeLineEnding(char *text)
{
    text[strcspn(text, "\r\n")] = '\0';
}

static int parseEmailLine(char *line, Email *email,
                          unsigned long arrival_order)
{
    char *sender;
    char *subject;
    char *date;

    if (strncmp(line, "EMAIL ", 6) != 0) {
        return 0;
    }

    sender = line + 6;
    subject = strchr(sender, ',');
    if (subject == NULL) {
        return 0;
    }
    *subject = '\0';
    subject++;

    date = strrchr(subject, ',');
    if (date == NULL) {
        return 0;
    }
    *date = '\0';
    date++;
    removeLineEnding(date);

    if (strlen(sender) >= SENDER_SIZE || strlen(subject) >= SUBJECT_SIZE ||
        strlen(date) >= DATE_SIZE) {
        return 0;
    }

    strcpy(email->sender, sender);
    strcpy(email->subject, subject);
    strcpy(email->date, date);
    email->sender_priority = getSenderPriority(sender);
    email->date_priority = getDatePriority(date);
    email->arrival_order = arrival_order;

    return email->sender_priority != 0 && email->date_priority != 0;
}

static void displayNextEmail(const MaxHeap *heap)
{
    const Email *email = peekMax(heap);

    if (email == NULL) {
        printf("No emails to read.\n\n");
        return;
    }

    printf("Next email:\n");
    printf("    Sender: %s\n", email->sender);
    printf("    Subject: %s\n", email->subject);
    printf("    Date: %s\n\n", email->date);
}

static void displayCount(const MaxHeap *heap)
{
    printf("There are %zu emails to read.\n\n", heap->size);
}

int main(int argc, char *argv[])
{
    FILE *input = stdin;
    MaxHeap inbox;
    char line[LINE_SIZE];
    unsigned long arrival_order = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [test_file]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            fprintf(stderr, "Could not open %s\n", argv[1]);
            return EXIT_FAILURE;
        }
    }

    initializeHeap(&inbox);

    while (fgets(line, sizeof(line), input) != NULL) {
        Email email;
        removeLineEnding(line);

        if (strncmp(line, "EMAIL ", 6) == 0) {
            if (!parseEmailLine(line, &email, arrival_order++)) {
                fprintf(stderr, "Invalid email line.\n");
                destroyHeap(&inbox);
                if (input != stdin) {
                    fclose(input);
                }
                return EXIT_FAILURE;
            }
            if (!insertEmail(&inbox, email)) {
                fprintf(stderr, "Not enough memory.\n");
                destroyHeap(&inbox);
                if (input != stdin) {
                    fclose(input);
                }
                return EXIT_FAILURE;
            }
        } else if (strcmp(line, "NEXT") == 0) {
            displayNextEmail(&inbox);
        } else if (strcmp(line, "READ") == 0) {
            removeMax(&inbox);
        } else if (strcmp(line, "COUNT") == 0) {
            displayCount(&inbox);
        }
    }

    destroyHeap(&inbox);
    if (input != stdin) {
        fclose(input);
    }

    return EXIT_SUCCESS;
}
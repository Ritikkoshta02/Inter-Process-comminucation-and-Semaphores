#include <pthread.h>
#include <unistd.h>
#include <bits/stdc++.h>
using namespace std;
const int EventsCount = 100;
const int  AudiCap = 500;
const int numOfWorkerThreads = 20;  
const int maxActiveQueries = 5;          
const int runningTime =100;
const int minTickets = 5;                 
const int maxTickets = 10;                
vector<vector<int>> sharedTable(maxActiveQueries,vector<int>(3,0)); // Shared
vector<int> availableSeats(EventsCount+1,  AudiCap);
pthread_mutex_t tableMutex;
pthread_cond_t tableCondition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t seatMutex;
void* workerThread(void* tid);
void inquireEvent(int eId, int tId);
int bookEvent(int eId, int seatsToBook, int tId);
int cancelEvent(int tId, vector<pair<int, int> >& bookings);
bool canRead(int eventId);
bool canWrite(int eventId);
int findBlankEntry();
int main(int argc, char const *argv[])
{
    cout<<endl;
    cout << "Event-reservation system for the Nehru Centre is STARTING:  "<<endl;
    pthread_mutex_init(&tableMutex, NULL);
    pthread_mutex_init(&seatMutex, NULL);
    // Create worker threads
    pthread_t threadIds[numOfWorkerThreads]; // Threads
    for (int i = 0; i < numOfWorkerThreads; i++) {
        void* t = &i;
        pthread_create(&threadIds[i], NULL, workerThread, t);
    }
    sleep(runningTime);
    // Signal worker threads to exit
    for (int i = 0; i < numOfWorkerThreads; i++)
        pthread_cancel(threadIds[i]);
    // Wait for worker threads to exit
    for (int i = 0; i < numOfWorkerThreads; i++)
        pthread_join(threadIds[i], NULL);
    // Print reservation status
    cout << "\n\n Final Status of Seats In All EVENTS at Nehru CENTER" << endl;
    for (int i = 1; i <= EventsCount; i++) {
        float perc = ( AudiCap - availableSeats[i])/(float) AudiCap * 100;
       cout<<endl<<"The Event No.: "<<setfill('0')<<setw(3)<<i<<" is "<<fixed << setprecision(2) <<perc<< "% booked.  The avilable Seat MATRIX for user is "<<setfill('0')<<setw(3)<<availableSeats[i]<<" seats avilable."<<endl;
    }
    pthread_mutex_destroy(&tableMutex);
    pthread_mutex_destroy(&seatMutex);
    pthread_cond_destroy(&tableCondition);
    return 0;
}
void* workerThread(void* tid) { // The purpose that every slave thread would carry out
    int threadId = *((int*) tid);
    srand(time(NULL) + threadId); 
    vector<pair<int, int> > bookings;
    while (true) {// until the master thread issues the exit command
        int queryType = rand() % 4 ;   
        int eventNum = rand() % EventsCount  + 1; 
         if(queryType==0) // ask to GET INFORMATION on a specific event
         { 
            inquireEvent(eventNum, threadId);
         }
         else if(queryType==1) // request to reserve a seat for an event 
         {
            int seatsToBook = rand() % (maxTickets - minTickets + 1) + minTickets; 
            int success = bookEvent(eventNum, seatsToBook, threadId); 
            if (success == 1) bookings.push_back(make_pair(seatsToBook, eventNum));  // The reservation will be saved in the local bookings vector if the booking is successful.
         }else
         {
            cancelEvent(threadId, bookings); // request to revoke a reservation for a performance
         }
        sleep(rand() % 3 +1);   
    }
}
void inquireEvent(int eId, int tId) {
    pthread_mutex_lock(&tableMutex); // locks the shared table to determine whether or not the query may be executed.
    int tableEntry = findBlankEntry(); // Finding the empty space to guarantee the most number of requests
    // Verify if the thread is able to read the seats for this occasion.
    // If not, wait for another thread to alert you.
    if (tableEntry == -1 || !canRead(eId)) {
        //Holding off till the necessary circumstances
        pthread_cond_wait(&tableCondition, &tableMutex);
    }
    tableEntry = findBlankEntry();
    if (tableEntry == -1) return;
    // Before launching the query, make changes to the shared table.
    sharedTable[tableEntry][0] = eId;
    sharedTable[tableEntry][1] = 0;
    sharedTable[tableEntry][2] = tId;
    pthread_mutex_unlock(&tableMutex); // Open the table.
    pthread_mutex_lock(&seatMutex);  // Since we would be reading the same material, get the seat locks.
    if (availableSeats[eId] == 0)
       cout<<"\n\nInquire Process => "<<"Thread No.: "<<setfill('0')<<setw(2)<<tId<<" in Event No.: "<<setfill('0')<<setw(3)<<eId<<" Seat MATRIX is Completely FULL..!";
    else
       cout<<endl<<"\n\nInquire Process => "<<"Thread No.: "<<setfill('0')<<setw(2)<<tId<<" in Event No.: "<<setfill('0')<<setw(3)<<eId<<"  There are "<<setfill('0')<<setw(3)<<availableSeats[eId]<<" seats still available for this EVENT.";
    pthread_mutex_unlock(&seatMutex);  // Release the vector seat.
    // To reverse the table entry, obtain the shared table mutex.
    pthread_mutex_lock(&tableMutex);
    sharedTable[tableEntry][0] = 0;
    // Take it apart.
    pthread_mutex_unlock(&tableMutex);
    // Alert other threads that you believe are waiting for this one to end.
    pthread_cond_signal(&tableCondition);
}
int bookEvent(int eId, int seatsToBook, int tId) {
    pthread_mutex_lock(&tableMutex); // checks whether the query can be executed by locking the shared table.
    int tableEntry = findBlankEntry(); // Finding the empty space to guarantee the most number of requests
    // Verify if the thread is able to read the seats for this occasion.
    // If not, wait for another thread to alert you.
    if (tableEntry == -1 || !canRead(eId)) {
        // Waiting till condition meets
        pthread_cond_wait(&tableCondition, &tableMutex);
    }
    tableEntry = findBlankEntry();
    if (tableEntry == -1) return -1; //If the amount of active queries above the permitted threshold, return
    // Before launching the query, update the shared table.
    sharedTable[tableEntry][0] = eId;
    sharedTable[tableEntry][1] = 1;
    sharedTable[tableEntry][2] = tId;
    // Open the table
    pthread_mutex_unlock(&tableMutex);
    // Obtain a seat lock
    pthread_mutex_lock(&seatMutex);
    if (seatsToBook > availableSeats[eId]) {
        cout<<"\n\n"<<"Book    Process => "<<"Thread No.: "<<setfill('0')<<setw(2)<<tId<<" in Event No.: "<<setfill('0')<<setw(3)<<eId<<" Enough seats are not available to book "<<setfill('0')<<setw(3)<<seatsToBook<<" seats.";
        pthread_mutex_unlock(&seatMutex); // Before going back, unlock
        return -1; // Unable to book
    }
    cout<<"\n\n"<<"Book    Process => "<<"Thread No.: "<<setfill('0')<<setw(2)<<tId<<" in Event No.: "<<setfill('0')<<setw(3)<<eId<<" has Booked "<<setfill('0')<<setw(2)<<seatsToBook<<" seats for this specific Event.";
    availableSeats[eId] -= seatsToBook; // Updating the seats for the event
    pthread_mutex_unlock(&seatMutex);  // Unlocking the seat
    // Get the shared table mutex so that you may change the table entry again.
    pthread_mutex_lock(&tableMutex);
    sharedTable[tableEntry][0] = 0;
    // Unclock it
    pthread_mutex_unlock(&tableMutex);
    // Alert other threads that you believe are waiting for this one to end.
    pthread_cond_signal(&tableCondition);
    return 1; 
}
int cancelEvent(int tId, vector<pair<int, int> >& bookings) {
    int totalBookings = bookings.size();
    if (totalBookings < 1) {
        cout<<"\n\n"<<"Cancel  Process => "<<"Thread No.: "<<setfill('0')<<setw(2)<<tId<<"  There is No such booking records in the Data Base of any Event by this thread.";
        return -1;
    }
    int pos = rand() % totalBookings  ;
    int numOfSeats = rand() %(bookings[pos].first) + 1; 
    int eId = bookings[pos].second; 
    // locks the shared table to determine whether or not the query may be executed.
    pthread_mutex_lock(&tableMutex);
    // Finding the empty space to guarantee the most number of requests
    int tableEntry = findBlankEntry();
    // Verify if the thread is able to read the seats for this occasion.
    // If not, wait for another thread to alert you.
    if (tableEntry == -1 || !canRead(eId)) {
        pthread_cond_wait(&tableCondition, &tableMutex);
    }
    tableEntry = findBlankEntry();
    if (tableEntry == -1) return -1;     // If there are more active queries than the permitted amount, please return.
    // Before launching the query, make changes to the shared table.
    sharedTable[tableEntry][0] = eId;
    sharedTable[tableEntry][1] = 2;
    sharedTable[tableEntry][2] = tId;
    // Unlock the table
    pthread_mutex_unlock(&tableMutex);
    // Lock the seat
    pthread_mutex_lock(&seatMutex);
    cout<<"\n\n"<<"Cancel  Process => "<<"Thread No.: "<<setfill('0')<<setw(2)<<tId<<" in Event No.: "<<setfill('0')<<setw(3)<<eId<<" has Canceled "<<setfill('0')<<setw(2)<<numOfSeats<<" seats for this event.";
    // Updating available seats
    availableSeats[eId] += numOfSeats;
    // Removing the booking from the threads list as it is cancelled
    bookings.erase(bookings.begin() + pos);
    // Unlocking the seats
    pthread_mutex_unlock(&seatMutex);
    // Resetting the shared table entry
    pthread_mutex_lock(&tableMutex);
    sharedTable[tableEntry][0] = 0;
    pthread_mutex_unlock(&tableMutex);
    pthread_cond_signal(&tableCondition);
    return 1;
}
bool canRead(int eventId) {
    for (int i = 0; i < maxActiveQueries; i++) {
        if (sharedTable[i][0] == eventId && sharedTable[i][1] != 0)
            return false;
    }
    return true;
}
bool canWrite(int eventId) {
    for (int i = 0; i < maxActiveQueries; i++) {
        if (sharedTable[i][0] == eventId)
            return false;
    }
    return true;
}
int findBlankEntry() {
    for (int i = 0; i < maxActiveQueries; i++) {
        if (sharedTable[i][0] == 0) {
            return i;
        }
    }
    return -1;
}

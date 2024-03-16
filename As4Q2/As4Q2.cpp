// An inter-process communication scheme for message passing between processes.
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
using namespace std;
bool comp(pair<int,int> p1, pair<int,int> p2)
{
    if(p1.second>p2.second)
    return true;
    return false;
}

struct Container {
    long type;
    char text[100];
};

int main() {
    
    int n, m;
    cout<<"Enter Number of Students Appearing"<<endl;
    cin >> n ;
    cout<<"Enter Number of Question's in Exam"<<endl;
    cin>>m;
    cout<<endl;
    
    pid_t pid[n];
    vector<int> Q_que(n), A_que(n) ; //These vectors will store the message queue IDs for sending questions (Q_que) and receiving answers (A_que) for each child process.
    vector<pair<int,int>> stud(n);
    
    for (int i = 0; i < n; i++) {
        Q_que[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
        A_que[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
        if (A_que[i] < 0 || Q_que[i] < 0) {
            cerr << "msgget failed" << endl;
            return 1;
        }
    }


    for (int i = 0; i < n; i++) {
        pid[i] = fork();
        
        if (pid[i] < 0) {
            
            cerr << "Fork failed" << endl;
            return 1;
            
        } else if (pid[i] == 0) {
            
            srand(time(NULL) + i + 1);
            Container question;
            Container answer;
            
            cout << "Child Processes Created" << endl;
            
            for (int j = 0; j < m; j++) {
                
                // Receive question from parent
                msgrcv(Q_que[i], &question, sizeof(Container) - sizeof(long), j + 1, 0);
                cout << "Child " <<setfill('0')<<setw(2)<< i + 1 << " : Question received - " << question.text << endl;

                // Answer question
                answer.text[0] = 'A'+ rand() % 4;
                answer.text[1] = '\0';
                cout << "Child " <<setfill('0')<<setw(2)<< i + 1 << " : Answer chosen for question - " << j + 1 <<  " is " << answer.text << endl;
                answer.type = question.type;

                // Send answer to parent
                msgsnd(A_que[i], &answer, sizeof(Container) - sizeof(long), 0);
                
            }

            // Receive final message from parent
            msgrcv(Q_que[i], &question, sizeof(Container) - sizeof(long), m, 0);
            cout << "Child " <<setfill('0')<<setw(2)<< i + 1 <<  " : Final message received - " << question.text << endl;
            exit(0);
        }
    }

    // Parent process
    Container question;
    Container answer;
    vector<int> scores(n, 0);
    vector<int> ans(m, rand() % 4);

    for (int j = 0; j < m; j++) {
        cout<<"Question Number:  "<< j+1<<endl;
        for (int i = 0; i < n; i++) {
            // Send question to child i
            stringstream ss;
            ss << "Question " << j + 1;
            strncpy(question.text, ss.str().c_str(), sizeof(question.text) - 1);
            question.text[sizeof(question.text) - 1] = '\0';
            question.type = j + 1;
            msgsnd(Q_que[i], &question, sizeof(Container) - sizeof(long), 0);

            // Receive answer from child i
            msgrcv(A_que[i], &answer, sizeof(Container) - sizeof(long), j + 1, 0);
            if (answer.text[0] == 'A'+ans[j])
                scores[i] += 5;
        }
         cout<<endl;
    }
    
    for(int i=0;i<n;i++)
    {
        stud[i]={i+1,scores[i]};
    }

    // Send final scores to children
    for (int i = 0; i < n; i++) {
        stringstream ss;
        ss << "Score: " <<scores[i];
        strncpy(question.text, ss.str().c_str(), sizeof(question.text) - 1);
        question.text[sizeof(question.text) - 1] = '\0';
        question.type = m;
        msgsnd(Q_que[i], &question, sizeof(Container) - sizeof(long), 0);
    }

    // Wait for child processes to finish
    for (int i = 0; i < n; i++) {
        waitpid(pid[i], NULL, 0);
        // Remove message queues
        int ret = msgctl(Q_que[i], IPC_RMID, NULL);
        if (ret == -1)
            cerr << "Error removing message queue Q_que[" << i << "]" << endl;
        ret = msgctl(A_que[i], IPC_RMID, NULL);
        if (ret == -1)
            cerr << "Error removing message queue A_que[" << i << "]" << endl;
    }
    
    cout <<endl <<"Answer Key :" << endl;
    for(int i=0;i<m;i++)
    {
        char c=ans[i]+'A';
        cout<<setfill('0')<<setw(2)<<i+1<<":"<<c<<", ";
    }
    cout<<endl;
    
    cout <<endl <<"Overall Grade distribution of Each Student :" << endl;
    cout<<endl;
    sort(stud.begin(),stud.end(),comp);
    int a=0,b=0,c=0,d=0,f=0;
    for(int i=0;i<n;i++)
    {
        if(i<=ceil((n*10)/100.0) && stud[i].second!=0)
       {
            cout<<"Student ID: "<<setfill('0')<<setw(2)<<stud[i].first<<" "<<"Score: "<<setfill(' ')<<setw(3)<<stud[i].second<<" Grade Recived: " << "A" <<endl;
            a++;
       }
        else if(i<=ceil((n*30)/100.0) && stud[i].second!=0)
        {
             cout<<"Student ID: "<<setfill('0')<<setw(2)<<stud[i].first<<" "<<"Score: "<<setfill(' ')<<setw(3)<<stud[i].second<<" Grade Recived: " << "B" <<endl;
             b++;
        }
         else if(i<=ceil((n*50)/100.0) && stud[i].second!=0)
        {
             cout<<"Student ID: "<<setfill('0')<<setw(2)<<stud[i].first<<" "<<"Score: "<<setfill(' ')<<setw(3)<<stud[i].second<<" Grade Recived: " << "C" <<endl;
             c++;
        }
         else if(i<=ceil((n*70)/100.0) && stud[i].second!=0)
        {
             cout<<"Student ID: "<<setfill('0')<<setw(2)<<stud[i].first<<" "<<"Score: "<<setfill(' ')<<setw(3)<<stud[i].second<<" Grade Recived: " << "D" <<endl;
             d++;
        }
        else
        {
            cout<<"Student ID: "<<setfill('0')<<setw(2)<<stud[i].first<<" "<<"Score: "<<setfill(' ')<<setw(3)<<stud[i].second<<" Grade Recived: " << "F" <<endl;
            f++;
        }
    }
    
    cout <<endl <<"Overall Grade distribution of EXAM:" << endl;
    cout<<"A :" <<a<<" B :"<<b<<" C :"<<c<<" D :"<<d <<" F :"<<f<<endl; 
    

    return 0;
}

#include <iostream>

using namespace std;

int main()
{
  int current = 0;
  int prev = -1;
  int answer = 0;
  int zero = 0;

  int input;
  
  
  cout << "Enter to find Fibonacci number: " << endl;
  cin >> input;
  
  for(int i = 0; i <= input; i++)
  {
     if(i == 0)
     {
       prev++;
       cout << prev << " "; 
       current++;
       cout << current << " ";
     }
     else
     {
        answer = prev + current;
        prev = current;
        current = answer;
        cout << answer << " ";
     }  
  }
  return 0;   
}

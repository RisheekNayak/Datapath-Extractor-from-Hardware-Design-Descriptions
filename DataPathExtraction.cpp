#include <bits/stdc++.h>

#define curtime chrono::high_resolution_clock::now()
#define timedif(start, end) chrono::duration_cast<chrono::nanoseconds>(end - start).count()

using namespace std;

struct Node
{
    string name_of_gate, type_of_gate;
    int output = -1;
    vector<string> incoming; // Inputs from other gates

    bool has_input = false;
    vector<string> input; // Direct inputs from in(i)

    int indegree = 0;
};

unordered_map<string, int> Inputs;       // Input wire to the circuit
unordered_map<string, string> Wire_Gate; // Map from the gate's output wire name to the gate's name
vector<string> output_wires;

void make_wire_gate_pair(string file_name)
{
    // Constructing Wire_Gate map.
    ifstream fin(file_name);
    string line;

    while (getline(fin, line))
    {
        istringstream iss(line);
        string first_word;
        iss >> first_word;

        if (first_word == "module" || first_word == "input" || first_word == "output" || first_word == "wire" || first_word == "endmodule")
            continue;

        string second_word;
        iss >> second_word;

        string name;
        int i = 0;
        for (; i < (int)second_word.size() && second_word[i] != '('; i++)
            name += second_word[i];

        string output;
        i++;
        for (; i < (int)second_word.size(); i++)
            output += second_word[i];

        // cout << output << " " << name << endl;
        if (output.substr(0, 3) == "out")
            output_wires.push_back(output);

        Wire_Gate[output] = name;
    }
}

unordered_map<string, vector<string>> GRAPH; // Map from parent's gate's name to children's gate's name.
unordered_map<string, Node *> Name_Gate;     // Map from gate's name to the node.

void construct_circuit(string file_name)
{
    ifstream fin(file_name);
    string line;

    while (getline(fin, line))
    {
        istringstream iss(line);
        string first_word;
        iss >> first_word;

        if (first_word == "module" || first_word == "input" || first_word == "output" || first_word == "wire" || first_word == "endmodule")
            continue;

        string second_word;
        iss >> second_word;

        string name;
        int i = 0;
        for (; i < (int)second_word.size() && second_word[i] != '('; i++)
            name += second_word[i];

        for (i = 0; i < (int)line.size() && line[i] != ','; i++)
            ;
        i++;

        string cur;
        while (line[i] != ';')
        {
            if (line[i] == ' ' || line[i] == ')')
            {
                auto child = new Node;

                if (Name_Gate.find(name) != Name_Gate.end())
                    child = Name_Gate[name];

                child->name_of_gate = name;
                child->type_of_gate = first_word;

                if (Wire_Gate.find(cur) == Wire_Gate.end()) // Doesnt Exist, its an input gate.
                {
                    child->has_input = true;
                    child->input.push_back(cur);
                    Inputs[cur] = 0;
                    // Inputs[cur] = rand() % 2; // Randomly initialising the input value.

                    if (cur == "1'b1")
                        Inputs[cur] = 1;
                }
                else
                {
                    string parent = Wire_Gate[cur];
                    child->incoming.push_back(parent);
                    GRAPH[parent].push_back(child->name_of_gate);
                }
                Name_Gate[name] = child;
                cur = "";
            }
            else if (line[i] != ',')
                cur += line[i];

            i += 1;
        }
    }
}

bool output_value(vector<int> &inputs, string type)
{
    // xnor, xor, and, nor, or, buf, not

    bool xor_value = 0, and_value = 1, or_value = 0;

    for (int i = 0; i < (int)inputs.size(); i++)
    {
        and_value = and_value && inputs[i];
        or_value = or_value || inputs[i];
        xor_value = xor_value ^ inputs[i];
    }

    if (type == "not")
        return !inputs[0];

    if (type == "xor")
        return xor_value;

    if (type == "xnor")
        return !xor_value;

    if (type == "or")
        return or_value;

    if (type == "nor")
        return !or_value;

    if (type == "and")
        return and_value;

    if (type == "nand")
        return !and_value;
    
    assert(type == "buf");
    return inputs[0];
}

vector<vector<string>> gates_per_level;
void simulation()
{
    // Topological Sorting
    for (auto parent : GRAPH)
        for (string child : parent.second)
            Name_Gate[child]->indegree++;

    queue<string> ready;

    for (auto parent : GRAPH)
        if (Name_Gate[parent.first]->indegree == 0)
            ready.push(parent.first);

    for (int size = ready.size(); !ready.empty(); size = ready.size())
    {
        vector<string> cur_level;
        while (size--)
        {
            string cur = ready.front();
            ready.pop();
            vector<int> inputs;
            cur_level.push_back(cur);

            for (auto it : Name_Gate[cur]->incoming)
            {
                bool ans = Name_Gate[it]->output;
                assert(ans != -1);
                inputs.push_back(ans);
            }

            if (Name_Gate[cur]->has_input)
            {
                for (auto it : Name_Gate[cur]->input)
                    inputs.push_back(Inputs[it]);
            }

            int value = output_value(inputs, Name_Gate[cur]->type_of_gate);
            Name_Gate[cur]->output = value;

            for (auto child : GRAPH[cur])
                if (--Name_Gate[child]->indegree == 0)
                    ready.push(child);
        }
        gates_per_level.push_back(cur_level);
    }

    // Gates having only input wires as their input and directly connected to the output wire.

    for (auto it : Wire_Gate)
    {
        string wire = it.first;
        if (wire.substr(0, 3) == "out")
        {
            auto Gate = Name_Gate[it.second];
            if (Gate->incoming.size() == 0)
            {
                vector<int> inputs;
                for (auto values : Gate->input)
                    inputs.push_back(Inputs[values]);

                int ans = output_value(inputs, Gate->type_of_gate);
                Name_Gate[it.second]->output = ans;
            }
        }
    }
}

void all_inputs(vector<string> &to_add, string cur, int number)
{
    if (cur.size() == number)
    {
        to_add.push_back(cur);
        return;
    }

    all_inputs(to_add, cur + '0', number);
    all_inputs(to_add, cur + '1', number);
}

unordered_map<string, string> gate_wire;
void reverse()
{
    for (auto i : Wire_Gate)
    {
        gate_wire[i.second] = i.first;
    }
}

unordered_map<string, string> relation;
string relation_wire(string wire)
{
    string gate = Wire_Gate[wire];

    Node *node = Name_Gate[gate];
    string type = node->type_of_gate;
    string gate1, gate2, final;

    if (node->input.size() == 0)
    {
        gate1 = node->incoming[0];

        if ((node->incoming).size() == 2)
        {
            gate2 = node->incoming[1];
            final = "( " + gate_wire[gate1] + " " + type + " " + gate_wire[gate2] + " )";
        }
        else
        {
            final = "( " + gate_wire[gate1] + " " + type + " )";
        }
    }
    else if (node->incoming.size() == 0)
    {
        gate1 = node->input[0];

        if ((node->input).size() == 2)
        {
            gate2 = node->input[1];
            final = "( " + gate1 + " " + type + " " + gate2 + " )";
        }
        else
        {
            final = "( " + gate1 + " " + type + " )";
        }
    }
    else
    {
        gate1 = node->input[0];
        gate2 = node->incoming[0];
        final = "( " + gate1 + " " + type + " " + gate_wire[gate2] + " )";
    }
    return final;
}

void logic()
{
    for (auto i : Wire_Gate)
    {
        string wire = i.first;
        relation[wire] = relation_wire(wire);
    }
}

int levels = gates_per_level.size();
unordered_map<string, string> logics;
void develop()
{
    for (int i = 0; i < gates_per_level[0].size(); i++)
        logics[gate_wire[gates_per_level[0][i]]] = relation[gate_wire[gates_per_level[0][i]]];
}

void form()
{
    for (int i = 1; i < gates_per_level.size(); i++)
    {
        for (int j = 0; j < gates_per_level[i].size(); j++)
        {
            int k = 0;
            string s = relation[gate_wire[gates_per_level[i][j]]];

            string temp, temp2;

            while (k < s.size())
            {
                if (s[k] != ' ')
                {
                    temp += s[k];
                }
                else
                {
                    if (logics.count(temp))
                    {

                        temp2 += logics[temp];
                        temp2 += " ";
                        temp = "";
                    }
                    else
                    {
                        temp2 += temp;
                        temp2 += " ";
                        temp = "";
                    }
                }
                k++;
            }
            if (temp != "")
            {
                if (logics.count(temp))
                {
                    temp2 += logics[temp];
                    temp = "";
                }
                else
                {
                    temp2 += temp;
                    temp = "";
                }
            }
            logics[gate_wire[gates_per_level[i][j]]] = temp2;
        }
    }
}

unordered_map<string, int> input_values;
int evaluate(string s)
{
    int stored_ans = 0;
    int num1 = -1;
    int num2 = -1;
    int i = 0;
    string temp;
    while (i < s.size())
    {
        if (s[i] != ' ')
        {
            temp += s[i];
        }
        else
        {
            if (input_values.count(temp))
            {
                if (num1 == -1)
                {
                    num1 = input_values[temp];
                }
                else
                {
                    num2 = input_values[temp];
                }
            }

            else
            {
                if (temp == "and")
                {
                    if (num1 != -1 && num2 == -1)
                    {
                        stored_ans = stored_ans & num1;
                    }
                    else
                    {
                        stored_ans = num1 & num2;
                    }
                }
                else if (temp == "not")
                {
                    if (num1 != -1)
                    {
                        stored_ans = !num1;
                    }
                    else
                    {
                        stored_ans = !stored_ans;
                    }
                }
                num1 = -1;
                num2 = -1;
                temp = "";
            }
        }
    }
    return stored_ans;
}

void print_gate_with_output_value()
{
    unordered_set<string> visited;
    for (auto parent : GRAPH)
    {
        if (visited.find(parent.first) == visited.end())
            cout << parent.first << " " << Name_Gate[parent.first]->output << endl, visited.insert(parent.first);

        for (string child : parent.second)
            if (visited.find(child) == visited.end())
                cout << child << " " << Name_Gate[child]->output << endl, visited.insert(child);
    }

    for (auto it : Wire_Gate)
    {
        string wire = it.first;
        if (wire.substr(0, 3) == "out")
        {
            auto Gate = Name_Gate[it.second];
            if (Gate->incoming.size() == 0)
                cout << Gate->name_of_gate << " " << Gate->output << endl;
        }
    }
}

void print_gate_with_level()
{
    cout << endl;
    for (int i = 0; i < (int)gates_per_level.size(); i++)
    {
        vector<string> it = gates_per_level[i];
        cout << "Level " << i << "(" << it.size() << " gates)"
             << ": ";
        for (auto &gate : it)
            cout << gate << " ";
        cout << "\n\n";
    }
}

void executeTestCases(int test)
{
    for (int curTest = 1; curTest <= test; curTest++)
    {
        auto time0 = curtime;
        
        string file_name = "top_primitive" + to_string(curTest) + ".v";
        make_wire_gate_pair(file_name);
        construct_circuit(file_name);
        reverse();
        simulation();

        // print_gate_with_output_value();
        // print_gate_with_level();

        logic();
        develop();
        form();

        ofstream fout("output" + to_string(curTest) + ".txt");
        for (auto &it : logics)
            if (it.first.substr(0, 3) == "out")
                fout << it.first << ": " << it.second << "\n";

        cout << "Execution Time of test case " + to_string(curTest) + ": " << timedif(time0, curtime) * 1e-9 << " sec\n";
    }
}

int main()
{
    srand(time(NULL)); // To randomize every run.
    executeTestCases(17);
    return 0;
}
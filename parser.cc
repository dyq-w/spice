
//  PARSER.CC  Ver. 1.0
//  Program to extract Nodal equations from Spice Netlist.  Ed Chan

#include "parser.h"
using namespace std;

double stripString(char* stringIn);
void printComponents(Component* compPtr);
void printNodes(Node* nodePtr, int compFlag);
char* strComponentType(Component* compPtr);
char* ComponentTypeName(Component* compPtr);  //obtain component type name
int portNum(Component* comPtr, Node* nodePtr); //obtain port number
bool isAccurate(double result[], int num, double accurateValue);
bool isClose(double A[], double B[], int num, double accurate = 1e-6);
void Fun(double A[][30], double x[], double b[], int n);
void convertArray(double jacMat[][30], double A[][30], double result[], double y[], int number);
void NR_Iterations(double jacMat[][30], double result[], double minDert[], int number, int& count, 
    double accurateValue, int datum, int lastnode, bool Homotopy_NeW = false, bool Homotopy = false, double t = 0);



NodeHead nodeList;
CompHead compList;



int main(int argc, char* argv[]) {
    ifstream inFile;
    ofstream outFile;
    ofstream outfile;       //add another 'outfile' to finish relevant work
    ModelHead modelList;

    // Buffers used in parsing:
    char inName[NameLength], outName[NameLength], buf[BufLength],myOutName[NameLength],
        buf1[BufLength], buf2[BufLength], buf3[BufLength], nameBuf[NameLength],
        * bufPtr, * charPtr1, * charPtr2;
    int intBuf1, intBuf2, intBuf3, intBuf4, datum = NA, eqNum = NA, specPrintJacMNA = 0, count=0;
    double douBuf1, douBuf2, douBuf3, douBuf4;
    CompType typeBuf;
    Component* compPtr, * compPtr1, * compPtr2;
    Node* nodePtr, * nodePtr1, * nodePtr2;
    Model* modelPtr;
    TranType TtypeBuf;
    EquaType eqType = Modified;

    strcpy(inName, "NOTHING");
    strcpy(outName, "NOTHING");
    strcpy(myOutName, "NOTHING");


    // process equation types:
    if (eqNum == NA) {
        while ((eqNum != 1) && (eqNum != 2)) {
            cout << "Available Equations Types Are:" << endl
                << " <1>  Nodal" << endl
                << " <2>  Modified Nodal" << endl
                << "Please enter your choice <1, 2>:" << endl;
            cin >> buf;
            eqNum = atoi(buf);
        }
        if (eqNum == 1)
            eqType = Nodal;
        else if (eqNum == 2)
            eqType = Modified;
    }

    // process input file name:
    if (!strcmp(inName, "NOTHING")) {
        cerr << "Please enter the input Spice Netlist: <QUIT to exit>" << endl;
        cin >> inName;
        if (!strcmp(inName, "QUIT")) {
            cerr << "Program Exited Abnormally!" << endl;
            exit(0);
        }
    }
    inFile.open(inName, ios::in);
    while (!inFile) {
        cerr << inName << " is an invalid input file." << endl
            << "Please enter the input Spice Netlist: <QUIT to exit>" << endl;
        cin >> inName;
        if (!strcmp(inName, "QUIT")) {
            cerr << "Program Exited Abnormally!" << endl;
            exit(0);
        }
        inFile.open(inName, ios::in);
    }

    // process output file
    if (!strcmp(outName, "NOTHING")) {
        strcpy(outName, inName);
        strtok(outName, ".");
        strcat(outName, ".Pout");
    }
    outFile.open(outName, ios::out);
    cout<< endl;


    // parsing of netlist to create linked list of models (remember to reset the fstream)
    inFile.getline(buf, BufLength);       // first line of netlist is discarded
    inFile.getline(buf, BufLength);


    while (inFile.good()) {
        if ((buf == NULL) || (*buf == '\0')) {
            inFile.getline(buf, BufLength);
            continue;
        }
        strcpy(buf1, buf);
        if (!strcmp(strtok(buf1, " "), ".model")) {
            strcpy(buf2, strtok(NULL, " "));
            charPtr1 = strtok(NULL, " ");
            if (!strcmp(charPtr1, "PNP"))
                TtypeBuf = PNP;
            else if (!strcmp(charPtr1, "NPN"))
                TtypeBuf = NPN;
            else if (!strcmp(charPtr1, "NMOS"))
                TtypeBuf = NMOS;
            else if (!strcmp(charPtr1, "PMOS"))
                TtypeBuf = PMOS;

            charPtr1 = strtok(NULL, " ");
            while (charPtr1 != NULL) {
                strcpy(buf3, "");
                if ((charPtr1[0] == 'I') && (charPtr1[1] == 'S') && (charPtr1[2] == '=')) {
                    douBuf1 = stripString(charPtr1);
                }
                if ((charPtr1[0] == 'B') && (charPtr1[1] == 'F') && (charPtr1[2] == '=')) {
                    douBuf2 = stripString(charPtr1);
                }
                if ((charPtr1[0] == 'B') && (charPtr1[1] == 'R') && (charPtr1[2] == '=')) {
                    douBuf3 = stripString(charPtr1);
                }
                if ((charPtr1[0] == 'T') && (charPtr1[1] == 'E') && (charPtr1[2] == '=')) {
                    douBuf4 = stripString(charPtr1);
                }
                charPtr1 = strtok(NULL, " ");
            }
            modelPtr = new Model(buf2, TtypeBuf, douBuf1, douBuf2, douBuf3, douBuf4);
            modelList.addModel(modelPtr);
        }
        inFile.getline(buf, BufLength);
    }
    inFile.close();
    inFile.open(inName, ios::in);



    //.Tran

    inFile.getline(buf, BufLength);       // first line of netlist is discarded
    inFile.getline(buf, BufLength);


    while (inFile.good()) {
        if ((buf == NULL) || (*buf == '\0')) {
            inFile.getline(buf, BufLength);
            continue;
        }
        strcpy(buf1, buf);
        if (!strcmp(strtok(buf1, " "), ".tran")) {
            
            charPtr1 = strtok(NULL, " ");
            stepSize = atof(charPtr1);
            
            charPtr1 = strtok(NULL, " ");
            stopTime = atof(charPtr1);
        }
        inFile.getline(buf, BufLength);
    }
    inFile.close();
    inFile.open(inName, ios::in);

    char model_str[9];
    //  starting of parsing by creating linked list of components
    inFile.getline(buf, BufLength);       // first line of netlist is discarded
    inFile.getline(buf, BufLength);
    while (inFile.good()) {
        if ((buf == NULL) || (*buf == '\0')) {
            inFile.getline(buf, BufLength);
            continue;
        }

        if (isalpha(*buf)) {

            //  EDIT THIS SECTION IF NEW COMPONENTS ARE ADDED!!!
            //  we could do some rearranging in this section to catch each type in order.
            switch (*buf) {
            case 'v':
            case 'V':
            {
                
                typeBuf = VSource;
                strcpy(nameBuf, strtok(buf, " "));
                intBuf1 = atoi(strtok(NULL, " "));
                intBuf2 = atoi(strtok(NULL, " "));
                douBuf1 = atof(strtok(NULL, " "));
                compPtr = new Component(typeBuf, douBuf1, NA, intBuf1, intBuf2, NA, NA, NULL, nameBuf);
                compList.addComp(compPtr);
                if (intBuf1 != datum && intBuf2 != datum) {
                    
                    Vsoure[compPtr->getcompNum()][0] = 1;
                    Vsoure[compPtr->getcompNum()][2] = intBuf2;
                }
                break;
            }
                
            case 'i':
            case 'I':
                cout << "I" << endl;
                typeBuf = ISource;
                strcpy(nameBuf, strtok(buf, " "));
                intBuf1 = atoi(strtok(NULL, " "));
                intBuf2 = atoi(strtok(NULL, " "));
                douBuf1 = atof(strtok(NULL, " "));
                compPtr = new Component(typeBuf, douBuf1, NA, intBuf1, intBuf2, NA, NA, NULL, nameBuf);
                compList.addComp(compPtr);
                break;
            case 'q':
            case 'Q':
                typeBuf = BJT;
                strcpy(nameBuf, strtok(buf, " "));
                intBuf1 = atoi(strtok(NULL, " "));
                intBuf2 = atoi(strtok(NULL, " "));
                intBuf3 = atoi(strtok(NULL, " "));
                compPtr = new Component(typeBuf, NA, NA, intBuf1, intBuf2, intBuf3, NA,
                    modelList.getModel(strtok(NULL, " ")), nameBuf);
                compList.addComp(compPtr);
                break;
            case 'm':
            case 'M':
                typeBuf = MOSFET;
                strcpy(nameBuf, strtok(buf, " "));
                intBuf1 = atoi(strtok(NULL, " "));
                intBuf2 = atoi(strtok(NULL, " "));
                intBuf3 = atoi(strtok(NULL, " "));
                intBuf4 = atoi(strtok(NULL, " "));
                compPtr = new Component(typeBuf, NA, NA, intBuf1, intBuf2, intBuf3, intBuf4,
                    modelList.getModel(strtok(NULL, " ")), nameBuf);
                compList.addComp(compPtr);
                break;
            case 'r':
            case 'R':
                typeBuf = Resistor;
                strcpy(nameBuf, strtok(buf, " "));
                intBuf1 = atoi(strtok(NULL, " "));
                intBuf2 = atoi(strtok(NULL, " "));
                douBuf1 = atof(strtok(NULL, " "));
                compPtr = new Component(typeBuf, douBuf1, NA, intBuf1, intBuf2, NA, NA, NULL, nameBuf);
                compList.addComp(compPtr);
                break;
            case 'd':
            case 'D':
                typeBuf = Diode;
                strcpy(nameBuf, strtok(buf, " "));
                intBuf1 = atoi(strtok(NULL, " "));
                intBuf2 = atoi(strtok(NULL, " "));
                charPtr1 = strtok(NULL, " ");
                while (charPtr1 != NULL) {
                    if ((charPtr1[0] == 'I') && (charPtr1[1] == 'S') && (charPtr1[2] == '=')) {
                        douBuf1 = stripString(charPtr1);
                    }
                    if ((charPtr1[0] == 'T') && (charPtr1[1] == 'E') && (charPtr1[4] == '=')) {
                        douBuf2 = stripString(charPtr1);
                    }
                    charPtr1 = strtok(NULL, " ");
                }
                compPtr = new Component(typeBuf, douBuf1, douBuf2, intBuf1, intBuf2, NA, NA, NULL, nameBuf);
                compList.addComp(compPtr);
                break;
            case 'c':
            case 'C':
                typeBuf = Capacitor;
                strcpy(nameBuf, strtok(buf, " "));
                intBuf1 = atoi(strtok(NULL, " "));
                intBuf2 = atoi(strtok(NULL, " "));
                douBuf1 = atof(strtok(NULL, " "));
                compPtr = new Component(typeBuf, douBuf1, NA, intBuf1, intBuf2, NA, NA, NULL, nameBuf);
                compList.addComp(compPtr);
                break;
            case 'l':
            case 'L':
                typeBuf = Inductor;
                strcpy(nameBuf, strtok(buf, " "));
                intBuf1 = atoi(strtok(NULL, " "));
                intBuf2 = atoi(strtok(NULL, " "));
                douBuf1 = atof(strtok(NULL, " "));
                compPtr = new Component(typeBuf, douBuf1, NA, intBuf1, intBuf2, NA, NA, NULL, nameBuf);
                compList.addComp(compPtr);
                break;
            };
        }
        inFile.getline(buf, BufLength);
    }


    //  Now the components are created and it is time to set up the list of nodes.
    //  we should actually use second connector of first Source as the first Node (Datum)
    compPtr1 = compList.getComp(0);
    while (compPtr1 != NULL) {
        for (int b = 0; b < 3; b++) { /* ~> J. Erik Melo note: A component can have until 4 connectors. But here just 3 are been considered. It should change the condition to 'b <= 3' or 'b < 4'?*/
            if ((!compPtr1->isCon(b)) && (compPtr1->getConVal(b) != NA)) { //~> verify if the connector 'b' is not set && if the name of the node to which this same connector 'b' is connected is a valid name as found in the circuit file. That is, if the name is not NA, that is, if this connector was named in the instantiation of the component.
                intBuf1 = compPtr1->getConVal(b); // ~> getting the connector number as in the netlist file
                nodePtr1 = nodeList.addNode();
                nodePtr1->setNameNum(intBuf1);  // ~> naming the node as in the netlist file
                compPtr1->connect(b, nodePtr1); // ~> connecting the 'connector' of component to the node
                nodePtr1->connect(b, compPtr1); // ~> connecting the 'connection' of the node to the component

                // now search and connect all other appropriate connectors to this node.
                // error checking should be added to prevent duplicated, or skipped connectors.
                compPtr2 = compPtr1->getNext();
                while (compPtr2 != NULL) {
                    for (int c = 0; c < 3; c++) { //~> verifying which one of the others connectors (of components) are connected to the node above
                        if (compPtr2->getConVal(c) == intBuf1) { //~> if next component in the list of components has a connector with the same name (conNum) of the connector above, connect it to the same node.
                            compPtr2->connect(c, nodePtr1);
                            nodePtr1->connect(c, compPtr2);
                            break;                                    //~> As a component can only have one connector with the same name (connected in the same node), don't search the others and go out of the 'for' loop
                        }
                    }
                    compPtr2 = compPtr2->getNext();
                }
            }
        }
        compPtr1 = compPtr1->getNext();
    }

    //  At this point, we are done creating a representation of the circuit in memory
    //  now, we need to call each node to create and output its nodal equation.
    //  Each node will call the components attached for the individual contributions to the
    //  nodal equation.

      // verify that input datum is valid
    Boolean check = FALSE;
    if (datum != NA) {
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() == datum)
                check = TRUE;
            nodePtr = nodePtr->getNext();
        }
        if (check == FALSE) {
            cerr << "Datum value invalid!" << endl
                << "PROGRAM EXITED ABNORMALLY!" << endl;
            exit(0);
        }
    }

    // Loop to find lastnode
    nodePtr = nodeList.getNode(0); //~> getting the pointer to the first node, pointed by 'headNode'
    int lastnode = nodePtr->getNameNum();
    while (nodePtr != NULL) {
        lastnode = (nodePtr->getNameNum() > lastnode) ? nodePtr->getNameNum() : lastnode;
        nodePtr = nodePtr->getNext();
    }
    //lastnode = 6;
   

    //  Loop to find the datum
    if (datum == NA) {
        nodePtr = nodeList.getNode(0);
        nodePtr1 = nodePtr->getNext();
        while (nodePtr1 != NULL) {
            if (nodePtr1->getCount() > nodePtr->getCount())
                nodePtr = nodePtr1;
            nodePtr1 = nodePtr1->getNext();
        }
        //datum = nodePtr->getNameNum();
        datum = 0; //此处做出了修改
    }

    //=================================
    //~> Checking the component list
    //~> Comment this part to omit
    compPtr = compList.getComp(0);
    printComponents(compPtr);

    nodePtr = nodeList.getNode(0);
    printNodes(nodePtr, 1);

    //<~
    //==================================

      // output circuit information
    outFile << "%Parser V1.0" << endl;
    outFile << "%Input Spice Deck:  " << inName << endl;
    outFile << "%Equation Type:     ";
    if (eqType == Nodal)
        outFile << "NODAL" << endl;
    else if (eqType == Modified)
        outFile << "MODIFIED NODAL" << endl;
    outFile << "%Datum Node:        " << datum << endl;


    // create value table
    outFile << endl
        << "%*****************************************************************************" << endl;
    outFile << "%                      Component Values:" << endl;
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->printVal(outFile);
        compPtr = compPtr->getNext();
    }
    outFile << endl
        << "%*****************************************************************************" << endl;


    // go down the nodal list and have components announce themselves
    outFile << endl << "%                      Circuit Equations: " << endl;
    nodePtr = nodeList.getNode(0);
    while (nodePtr != NULL) {
        if (nodePtr->getNameNum() != datum) {
            nodePtr->printNodal(outFile, datum , lastnode);
        }
        nodePtr = nodePtr->getNext();
    }

    //go down the component list and give equations for all sources
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->specialPrint(outFile, datum);
        compPtr = compPtr->getNext();
    }

    //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
    if (eqType != Modified) {
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->printSuperNode(outFile, datum, lastnode);
            compPtr = compPtr->getNext();
        }
    }

    // go down the node list and give additional MNA equations
    if (eqType == Modified) {
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum)
                nodePtr->printMNA(outFile, datum, lastnode);
            nodePtr = nodePtr->getNext();
        }
    }

    // print jacobians
    outFile << endl
        << "%*****************************************************************************" << endl;
    outFile << endl << "%                      Jacobians: " << endl;
    nodePtr1 = nodeList.getNode(0);
    while (nodePtr1 != NULL) {   //~> this loop handles the nodes not connected to a Vsource and those ones that are not the 'datum' node
        if (nodePtr1->getNameNum() != datum) {
            nodePtr2 = nodeList.getNode(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum) {
                    nodePtr1->printJac(outFile, datum, nodePtr2, lastnode, eqType);
                }
                nodePtr2 = nodePtr2->getNext();
            }
        }
        nodePtr1 = nodePtr1->getNext();
    }

    // go down the component list and give equations for all sources
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        nodePtr2 = nodeList.getNode(0);
        compPtr2 = compList.getComp(0);
        while (nodePtr2 != NULL) {
            if (nodePtr2->getNameNum() != datum) {
                compPtr->specialPrintJac(outFile, datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA ); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
            }
            nodePtr2 = nodePtr2->getNext();
        }
        specPrintJacMNA = 0;
        compPtr = compPtr->getNext();
    }



    // print the Jacobians for the additional MNA equations
    if (eqType == Modified) {
        nodePtr1 = nodeList.getNode(0);
        while (nodePtr1 != NULL) {
            if (nodePtr1->getNameNum() != datum) {
                nodePtr2 = nodeList.getNode(0);
                while (nodePtr2 != NULL) {
                    if (nodePtr2->getNameNum() != datum)
                        
                        nodePtr1->printJacMNA(outFile, datum, nodePtr2, lastnode);
                    nodePtr2 = nodePtr2->getNext();
                }
            }
            nodePtr1 = nodePtr1->getNext();
        }
    }


    cout << endl;


    // %***************************************************************************************************
/*
   if (!strcmp(myOutName, "NOTHING")) {
        strcpy(myOutName, inName);
        strtok(myOutName, ".");
        strcat(myOutName, "out.txt");
    }
    outfile.open(myOutName, ios::out);


    nodePtr = nodeList.getNode(0);

    outfile << "datum = " << datum << "\t\t" << "lastnode = " << lastnode << endl;
    Connections* conPtr;
    while (nodePtr != NULL) {

        outfile << "节点 " << nodePtr->getNameNum() << "\t\t" << "所连器件数为：" << nodePtr->getCount() << endl;
        conPtr = nodePtr->getConList();
        while (conPtr->next != NULL) {
            outfile << "\t\t" << "编号： " << conPtr->comp->getcompNum() << "\t\t" << "类型： " << ComponentTypeName(conPtr->comp) << "\t\t" << "链接端口：" << portNum(conPtr->comp, nodePtr) << "\t\t";
            switch (conPtr->comp->getType()) {
            case VSource:
                outfile << "名称：" << "VCC" << endl; break;
            default:
                outfile << "名称：" << strComponentType(conPtr->comp) << conPtr->comp->getcompNum() << endl; break;
            }
            outfile << "\t\t" << "value:" << conPtr->comp->getVal() << endl;

            conPtr = conPtr->next;
        }

        outfile << "\t\t" << "编号： " << conPtr->comp->getcompNum() << "\t\t" << "类型： " << ComponentTypeName(conPtr->comp) << "\t\t" << "链接端口：" << portNum(conPtr->comp, nodePtr) << "\t\t";
        switch (conPtr->comp->getType()) {
        case VSource:
            outfile << "名称：" << "VCC" << endl; break;
        default:
            outfile << "名称：" << strComponentType(conPtr->comp) << conPtr->comp->getcompNum() << endl;
            break;
        }
        outfile << "\t\t" << "value:" << conPtr->comp->getVal() << endl;

        nodePtr = nodePtr->getNext();
    }


    //输出KCL/KVL方程

    outfile << endl
        << "%*****************************************************************************" << endl;


    // go down the nodal list and have components announce themselves
    outfile << endl << "  KCL/KVL 方程 : " << endl;
    nodePtr = nodeList.getNode(0);
    while (nodePtr != NULL) {
        if (nodePtr->getNameNum() != datum) {
            nodePtr->printNodal(outfile, datum, lastnode);
        }
        nodePtr = nodePtr->getNext();
    }

    //go down the component list and give equations for all sources
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->specialPrint(outfile, datum);
        compPtr = compPtr->getNext();
    }

    //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
    if (eqType != Modified) {
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->printSuperNode(outfile, datum, lastnode);
            compPtr = compPtr->getNext();
        }
    }


    // go down the node list and give additional MNA equations
    if (eqType == Modified) {
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum)
                nodePtr->printMNA(outfile, datum, lastnode);
            nodePtr = nodePtr->getNext();
        }
    }

    outfile.close();*/

    //%****************************************************************************************************
    int choose = 0;

    cout << "*************************************" << endl;

    cout << "   1、N-R" << endl;
    cout << "   2、Homotopy-Newton" << endl;
    cout << "   3、Homotopy" << endl;
    cout << "   4、Transient" << endl;
    cout << "   5、CL_P_Transient" << endl;
    cout << "   6、CV_P_Transient" << endl;
    cout << "   7、CV_P_Transient_plus" << endl;

    cout << "*************************************" << endl;

    cin >> choose;

    if (choose == 1) {
        //以下实现矩阵的运算

        cout << endl << endl << "----------------Using N-R Method to Solve Circuit Equations--------------------------" << endl << endl;

        int number = 0;

        cout << "Please enter the initial data number：" << endl;
        cin >> number;
        cout << "Please enter the initial data value:" << endl;
        for (int i = 0; i < number; i++) {
            cin >> nodeValue[i + 1];
        }


        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum) {
                nodePtr->printNodalMat(datum, lastnode, result);
            }
            nodePtr = nodePtr->getNext();
        }

        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->specialPrintMat(datum, result);
            compPtr = compPtr->getNext();
        }


        //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
        if (eqType != Modified) {
            compPtr = compList.getComp(0);
            while (compPtr != NULL) {
                compPtr->printSuperNodeMat(datum, lastnode, result);
                compPtr = compPtr->getNext();
            }
        }


        // go down the node list and give additional MNA equations
        if (eqType == Modified) {
            nodePtr = nodeList.getNode(0);
            while (nodePtr != NULL) {
                if (nodePtr->getNameNum() != datum)
                    nodePtr->printMNAMat(datum, lastnode, result);
                nodePtr = nodePtr->getNext();
            }
        }





        nodePtr1 = nodeList.getNode(0);
        while (nodePtr1 != NULL) {
            if (nodePtr1->getNameNum() != datum) {
                nodePtr2 = nodeList.getNode(0);
                while (nodePtr2 != NULL) {
                    if (nodePtr2->getNameNum() != datum) {
                        nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
                    }
                    nodePtr2 = nodePtr2->getNext();
                }
            }
            nodePtr1 = nodePtr1->getNext();
        }

        // go down the component list and give equations for all sources
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            nodePtr2 = nodeList.getNode(0);
            compPtr2 = compList.getComp(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum) {
                    compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
                }
                nodePtr2 = nodePtr2->getNext();
            }
            specPrintJacMNA = 0;
            compPtr = compPtr->getNext();
        }




        // print the Jacobians for the additional MNA equations
        if (eqType == Modified) {
            nodePtr1 = nodeList.getNode(0);
            while (nodePtr1 != NULL) {
                if (nodePtr1->getNameNum() != datum) {
                    nodePtr2 = nodeList.getNode(0);
                    while (nodePtr2 != NULL) {
                        if (nodePtr2->getNameNum() != datum)
                            nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                        nodePtr2 = nodePtr2->getNext();
                    }
                }
                nodePtr1 = nodePtr1->getNext();
            }
        }


        int count = 1;
        double accurateValue;

        cout << "please input required accuracy:" << endl;
        cin >> accurateValue;
        cout << "------------------output------------------------------------" << endl;

      
        

        NR_Iterations(jacMat, result, minDert, number, count, accurateValue, datum, lastnode);



        cout << "iteration number:" << "  " << count << endl;
        cout << endl;
        for (int i = 0; i < number; i++) {
            cout << "▲x(" << i + 1 << ") =    " << minDert[i] << endl;
        }
        cout << endl;
        cout << "the result:" << endl;
        for (int i = 0; i < number; i++) {
            cout << "x(" << i + 1 << ") =    " << nodeValue[i + 1] << endl;
        }


    }
else if (choose == 2) {


//------------------------同伦法求解电路方程------------------------------

cout << endl << endl << "----------------Using Homotopy-Newton Method to Solve Circuit Equations--------------------------" << endl << endl;

outfile.open("nrhomo.txt", ios::out);
double stepSize;
int number = 0;

cout << "Please enter the initial data number：" << endl;
cin >> number;
double t = 0;
cout << "Please enter the step size:" << endl;
cin >> stepSize;
t = t + stepSize;
cout << "Please enter the initial data value:" << endl;
for (int i = 0; i < number; i++) {
    cin >> nodeValue[i + 1];
}
double accurateValue;

cout << "please input required accuracy:" << endl;
cin >> accurateValue;


//求F(x0)

nodePtr = nodeList.getNode(0);
while (nodePtr != NULL) {
    if (nodePtr->getNameNum() != datum) {
        nodePtr->printNodalMat(datum, lastnode, result);
    }
    nodePtr = nodePtr->getNext();
}

compPtr = compList.getComp(0);
while (compPtr != NULL) {
    compPtr->specialPrintMat(datum, result);
    compPtr = compPtr->getNext();
}


//~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
if (eqType != Modified) {
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->printSuperNodeMat(datum, lastnode, result);
        compPtr = compPtr->getNext();
    }
}


// go down the node list and give additional MNA equations
if (eqType == Modified) {
    nodePtr = nodeList.getNode(0);
    while (nodePtr != NULL) {
        if (nodePtr->getNameNum() != datum)
            nodePtr->printMNAMat(datum, lastnode, result);
        nodePtr = nodePtr->getNext();
    }
}

//求jac矩阵

nodePtr1 = nodeList.getNode(0);
while (nodePtr1 != NULL) {
    if (nodePtr1->getNameNum() != datum) {
        nodePtr2 = nodeList.getNode(0);
        while (nodePtr2 != NULL) {
            if (nodePtr2->getNameNum() != datum) {
                nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
            }
            nodePtr2 = nodePtr2->getNext();
        }
    }
    nodePtr1 = nodePtr1->getNext();
}

// go down the component list and give equations for all sources
compPtr = compList.getComp(0);
while (compPtr != NULL) {
    nodePtr2 = nodeList.getNode(0);
    compPtr2 = compList.getComp(0);
    while (nodePtr2 != NULL) {
        if (nodePtr2->getNameNum() != datum) {
            compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
        }
        nodePtr2 = nodePtr2->getNext();
    }
    specPrintJacMNA = 0;
    compPtr = compPtr->getNext();
}




// print the Jacobians for the additional MNA equations
if (eqType == Modified) {
    nodePtr1 = nodeList.getNode(0);
    while (nodePtr1 != NULL) {
        if (nodePtr1->getNameNum() != datum) {
            nodePtr2 = nodeList.getNode(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum)
                    nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                nodePtr2 = nodePtr2->getNext();
            }
        }
        nodePtr1 = nodePtr1->getNext();
    }
}

for (int i = 1; i <= number; i++) {
    initF[i] = result[i];
    result[i] = t * initF[i];
}

while (t < 1.0+stepSize) {
    NR_Iterations(jacMat, result, minDert, number, count, accurateValue, datum, lastnode, true, false, t);
    t += stepSize;
    
    nodePtr = nodeList.getNode(0);
    while (nodePtr != NULL) {
        if (nodePtr->getNameNum() != datum) {
            nodePtr->printNodalMat(datum, lastnode, result);
        }
        nodePtr = nodePtr->getNext();
    }

    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->specialPrintMat(datum, result);
        compPtr = compPtr->getNext();
    }


    //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
    if (eqType != Modified) {
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->printSuperNodeMat(datum, lastnode, result);
            compPtr = compPtr->getNext();
        }
    }


    // go down the node list and give additional MNA equations
    if (eqType == Modified) {
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum)
                nodePtr->printMNAMat(datum, lastnode, result);
            nodePtr = nodePtr->getNext();
        }
    }

    //求jac矩阵

    nodePtr1 = nodeList.getNode(0);
    while (nodePtr1 != NULL) {
        if (nodePtr1->getNameNum() != datum) {
            nodePtr2 = nodeList.getNode(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum) {
                    nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
                }
                nodePtr2 = nodePtr2->getNext();
            }
        }
        nodePtr1 = nodePtr1->getNext();
    }

    // go down the component list and give equations for all sources
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        nodePtr2 = nodeList.getNode(0);
        compPtr2 = compList.getComp(0);
        while (nodePtr2 != NULL) {
            if (nodePtr2->getNameNum() != datum) {
                compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
            }
            nodePtr2 = nodePtr2->getNext();
        }
        specPrintJacMNA = 0;
        compPtr = compPtr->getNext();
    }




    // print the Jacobians for the additional MNA equations
    if (eqType == Modified) {
        nodePtr1 = nodeList.getNode(0);
        while (nodePtr1 != NULL) {
            if (nodePtr1->getNameNum() != datum) {
                nodePtr2 = nodeList.getNode(0);
                while (nodePtr2 != NULL) {
                    if (nodePtr2->getNameNum() != datum)
                        nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                    nodePtr2 = nodePtr2->getNext();
                }
            }
            nodePtr1 = nodePtr1->getNext();
        }
    }
    if (t < 1.0+stepNum) {
        for (int i = 1; i <= number; i++) {
            result[i] = result[i] - (1 - t) * initF[i];
        }
    }


    outfile << "time:" << t-stepSize << endl;
    for (int i = 1;  i <= number; i++) {
        outfile << "x" << i << "=" << nodeValue[i] << endl;
    }
         
    


}
outfile.close();
//NR_Iterations(jacMat, result, minDert, number, count, accurateValue, datum, lastnode);

cout << endl;
for (int i = 0; i < number; i++) {
    cout << "▲x(" << i + 1 << ") =    " << minDert[i] << endl;
}
cout << endl;
cout << "the result:" << endl;
for (int i = 0; i < number; i++) {
    cout << "x(" << i + 1 << ") =    " << nodeValue[i + 1] << endl;
}

    }




else if (choose == 3) {
/*
    # 同伦法的公式      H(x,t)=tf(x)+(1-t)G(x-a)
    # 默认G为对角矩阵，且对角线的值为1e-3.
    # 默认 a 给定
    
*/

cout << endl << endl << "----------------Using Homotopy Method to Solve Circuit Equations--------------------------" << endl << endl;
outfile.open("homotopy.txt", ios::out);
double t_stepSize;
int number = 0, step_num = 0, step_sum=0;


cout << "Please enter the initial data number：" << endl;
cin >> number;
double t = 0;
cout << "Please enter the initial step size:" << endl;
cin >> t_stepSize;

double accurateValue =0.0;

cout << "please input required accuracy:" << endl;
cin >> accurateValue;


//第一次迭代会使，初始的nodeValued的值和a中的值相等
for (int i = 0; i < number; i++) {
     nodeValue[i + 1] = a[i+1];
}


t = t + t_stepSize;


//第二次迭代


//求H(x),存于result

nodePtr = nodeList.getNode(0);
while (nodePtr != NULL) {
    if (nodePtr->getNameNum() != datum) {
        nodePtr->printNodalMat(datum, lastnode, result);
    }
    nodePtr = nodePtr->getNext();
}

compPtr = compList.getComp(0);
while (compPtr != NULL) {
    compPtr->specialPrintMat(datum, result);
    compPtr = compPtr->getNext();
}


//~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
if (eqType != Modified) {
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->printSuperNodeMat(datum, lastnode, result);
        compPtr = compPtr->getNext();
    }
}


// go down the node list and give additional MNA equations
if (eqType == Modified) {
    nodePtr = nodeList.getNode(0);
    while (nodePtr != NULL) {
        if (nodePtr->getNameNum() != datum)
            nodePtr->printMNAMat(datum, lastnode, result);
        nodePtr = nodePtr->getNext();
    }
}

//求jac矩阵

nodePtr1 = nodeList.getNode(0);
while (nodePtr1 != NULL) {
    if (nodePtr1->getNameNum() != datum) {
        nodePtr2 = nodeList.getNode(0);
        while (nodePtr2 != NULL) {
            if (nodePtr2->getNameNum() != datum) {
                nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
            }
            nodePtr2 = nodePtr2->getNext();
        }
    }
    nodePtr1 = nodePtr1->getNext();
}

// go down the component list and give equations for all sources
compPtr = compList.getComp(0);
while (compPtr != NULL) {
    nodePtr2 = nodeList.getNode(0);
    compPtr2 = compList.getComp(0);
    while (nodePtr2 != NULL) {
        if (nodePtr2->getNameNum() != datum) {
            compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
        }
        nodePtr2 = nodePtr2->getNext();
    }
    specPrintJacMNA = 0;
    compPtr = compPtr->getNext();
}




// print the Jacobians for the additional MNA equations
if (eqType == Modified) {
    nodePtr1 = nodeList.getNode(0);
    while (nodePtr1 != NULL) {
        if (nodePtr1->getNameNum() != datum) {
            nodePtr2 = nodeList.getNode(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum)
                    nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                nodePtr2 = nodePtr2->getNext();
            }
        }
        nodePtr1 = nodePtr1->getNext();
    }
}

//对result 和 jacmat 进行处理


for (int i = 1; i <= number; i++) {

    result[i] = t * result[i] + (1 - t) * G * (nodeValue[i] - a[i]);

}



for (int i = 1; i <= number; i++) {
    for (int j = 1; j <= number; j++) {
        if (i == j) {

            jacMat[i][j] = jacMat[i][j] * t + (1 - t) * G;

        }
        else {
            jacMat[i][j] = jacMat[i][j] * t;
        }
        
    }
}


while (true) {
    NR_Iterations(jacMat, result, minDert, number, count, accurateValue, datum, lastnode, false, true, t);
    cout << endl << "第" << ++step_num << "次迭代次数：" << count << endl;
    step_sum += count;
  
    if (t == 1)
        break;
    if (count < 5) {
        t_stepSize *= 2;
        t = t + t_stepSize;
        
        for (int i = 1; i <= number; i++) {
            preU[i] = nodeValue[i];
        }
    }
    if (count > 4 && count < 8) {
        t += t_stepSize;
      
        for (int i = 1; i <= number; i++) {
            preU[i] = nodeValue[i];
        }
    }
    if (count > 7) {
        step_sum -= count;
        t -= t_stepSize;
        t_stepSize /= 2;
        t += t_stepSize;
        for (int i = 1; i <= number; i++) {
            nodeValue[i]= preU[i];
        }
    }
    
    //t = t + t_stepSize;
    

    if (t > 1)
        t = 1;

    cout << "t:" << t << endl;

    nodePtr = nodeList.getNode(0);
    while (nodePtr != NULL) {
        if (nodePtr->getNameNum() != datum) {
            nodePtr->printNodalMat(datum, lastnode, result);
        }
        nodePtr = nodePtr->getNext();
    }

    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->specialPrintMat(datum, result);
        compPtr = compPtr->getNext();
    }


    //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
    if (eqType != Modified) {
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->printSuperNodeMat(datum, lastnode, result);
            compPtr = compPtr->getNext();
        }
    }


    // go down the node list and give additional MNA equations
    if (eqType == Modified) {
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum)
                nodePtr->printMNAMat(datum, lastnode, result);
            nodePtr = nodePtr->getNext();
        }
    }

    //求jac矩阵

    nodePtr1 = nodeList.getNode(0);
    while (nodePtr1 != NULL) {
        if (nodePtr1->getNameNum() != datum) {
            nodePtr2 = nodeList.getNode(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum) {
                    nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
                }
                nodePtr2 = nodePtr2->getNext();
            }
        }
        nodePtr1 = nodePtr1->getNext();
    }

    // go down the component list and give equations for all sources
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        nodePtr2 = nodeList.getNode(0);
        compPtr2 = compList.getComp(0);
        while (nodePtr2 != NULL) {
            if (nodePtr2->getNameNum() != datum) {
                compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
            }
            nodePtr2 = nodePtr2->getNext();
        }
        specPrintJacMNA = 0;
        compPtr = compPtr->getNext();
    }




    // print the Jacobians for the additional MNA equations
    if (eqType == Modified) {
        nodePtr1 = nodeList.getNode(0);
        while (nodePtr1 != NULL) {
            if (nodePtr1->getNameNum() != datum) {
                nodePtr2 = nodeList.getNode(0);
                while (nodePtr2 != NULL) {
                    if (nodePtr2->getNameNum() != datum)
                        nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                    nodePtr2 = nodePtr2->getNext();
                }
            }
            nodePtr1 = nodePtr1->getNext();
        }
    }
    if (t <1.0 || t==1) {
        for (int i = 1; i <= number; i++) {
            result[i] = t * result[i] + (1 - t) * G * (nodeValue[i] - a[i]);
        }

        for (int i = 1; i <= number; i++) {
            for (int j = 1; j <= number; j++) {
                if (i == j) {

                    jacMat[i][j] = jacMat[i][j] * t + (1 - t) * G;

                }
                else {
                    jacMat[i][j] = jacMat[i][j] * t;
                }

            }
        }
    }

    outfile << "time:" << t-t_stepSize << endl;
    for (int i = 1; i <= number; i++) {
        outfile << "x" << i << "=" << nodeValue[i] << endl;
    }


}

cout << "N_R迭代总次数：" << step_sum << endl;
outfile.close();
cout << endl;
for (int i = 0; i < number; i++) {
    cout << "▲x(" << i + 1 << ") =    " << minDert[i] << endl;
}
cout << endl;
cout << "the result:" << endl;
for (int i = 0; i < number; i++) {
    cout << "x(" << i + 1 << ") =    " << nodeValue[i + 1] << endl;
}









}


else if (choose == 4) {

// -------------------------------Tran---------------------------------------------------

cout << endl << endl << "----------------Using Tran Method to Solve Circuit Equations--------------------------" << endl << endl;


outfile.open("tran.txt", ios::out);
isTran = 1;              //--------------------
int number = 3;
nodeValue[2] = 0;  
nodeValue[1] = 0;
nodeValue[3] = 0;

double accurateValue;

cout << "please input required accuracy:" << endl;
cin >> accurateValue;

for (double i = stepSize; i < stopTime+stepSize; i = i + stepSize) {
    stepNum++;
    nodePtr = nodeList.getNode(0);
    while (nodePtr != NULL) {
        if (nodePtr->getNameNum() != datum) {
            nodePtr->printNodalMat(datum, lastnode, result);
        }
        nodePtr = nodePtr->getNext();
    }

    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->specialPrintMat(datum, result);
        compPtr = compPtr->getNext();
    }


    //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
    if (eqType != Modified) {
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->printSuperNodeMat(datum, lastnode, result);
            compPtr = compPtr->getNext();
        }
    }


    // go down the node list and give additional MNA equations
    if (eqType == Modified) {
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum)
                nodePtr->printMNAMat(datum, lastnode, result);
            nodePtr = nodePtr->getNext();
        }
    }

    //求jac矩阵

    nodePtr1 = nodeList.getNode(0);
    while (nodePtr1 != NULL) {
        if (nodePtr1->getNameNum() != datum) {
            nodePtr2 = nodeList.getNode(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum) {
                    nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
                }
                nodePtr2 = nodePtr2->getNext();
            }
        }
        nodePtr1 = nodePtr1->getNext();
    }

    // go down the component list and give equations for all sources
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        nodePtr2 = nodeList.getNode(0);
        compPtr2 = compList.getComp(0);
        while (nodePtr2 != NULL) {
            if (nodePtr2->getNameNum() != datum) {
                compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
            }
            nodePtr2 = nodePtr2->getNext();
        }
        specPrintJacMNA = 0;
        compPtr = compPtr->getNext();
    }




    // print the Jacobians for the additional MNA equations
    if (eqType == Modified) {
        nodePtr1 = nodeList.getNode(0);
        while (nodePtr1 != NULL) {
            if (nodePtr1->getNameNum() != datum) {
                nodePtr2 = nodeList.getNode(0);
                while (nodePtr2 != NULL) {
                    if (nodePtr2->getNameNum() != datum)
                        nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                    nodePtr2 = nodePtr2->getNext();
                }
            }
            nodePtr1 = nodePtr1->getNext();
        }
    }
    
    for (int i = 0; i <= number; i++) {
        preU[i] = nodeValue[i];
    }
    NR_Iterations(jacMat, result, minDert, number, count, accurateValue, datum, lastnode);

    outfile << "Time:" << i << "    voltage:" << nodeValue[2] << endl;
}

outfile.close();

    }
else if (choose == 5) {


//----------------------------P_Tran-----------------------------------------------------------

//同时加入电容和电感
cout << endl << endl << "----------------Using P-Tran Method to Solve Circuit Equations--------------------------" << endl << endl;

outfile.open("ptran.txt", ios::out);
int number = 0;
isTran = 1;      //--------------------


cout << "please input data numbers:" << endl;
cin >> number;
for (int i = 1; i <= number; i++) {
    nodeValue[i] = 0.0;
}

double accurateValue = 0.0;

cout << "please input required accuracy:" << endl;
cin >> accurateValue;

//for (double i = stepSize; i < stopTime + stepSize; i = i + stepSize) {
while (true) {

    stepNum++;

    endTimeFlag += stepSize;
    if (endTimeFlag > stopTime)
         throw runtime_error("timeout");
     
    if (stepSize < 1e-6)
        throw runtime_error("stepSize is too small!");

  
    nodePtr = nodeList.getNode(0);
    while (nodePtr != NULL) {
        if (nodePtr->getNameNum() != datum) {
            nodePtr->printNodalMat(datum, lastnode, result);
        }
        nodePtr = nodePtr->getNext();
    }

    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->specialPrintMat(datum, result);
        compPtr = compPtr->getNext();
    }


    //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
    if (eqType != Modified) {
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->printSuperNodeMat(datum, lastnode, result);
            compPtr = compPtr->getNext();
        }
    }


    // go down the node list and give additional MNA equations
    if (eqType == Modified) {
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum)
                nodePtr->printMNAMat(datum, lastnode, result);
            nodePtr = nodePtr->getNext();
        }
    }

    //求jac矩阵

    nodePtr1 = nodeList.getNode(0);
    while (nodePtr1 != NULL) {
        if (nodePtr1->getNameNum() != datum) {
            nodePtr2 = nodeList.getNode(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum) {
                    nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
                }
                nodePtr2 = nodePtr2->getNext();
            }
        }
        nodePtr1 = nodePtr1->getNext();
    }

    // go down the component list and give equations for all sources
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        nodePtr2 = nodeList.getNode(0);
        compPtr2 = compList.getComp(0);
        while (nodePtr2 != NULL) {
            if (nodePtr2->getNameNum() != datum) {
                compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
            }
            nodePtr2 = nodePtr2->getNext();
        }
        specPrintJacMNA = 0;
        compPtr = compPtr->getNext();
    }




    // print the Jacobians for the additional MNA equations
    if (eqType == Modified) {
        nodePtr1 = nodeList.getNode(0);
        while (nodePtr1 != NULL) {
            if (nodePtr1->getNameNum() != datum) {
                nodePtr2 = nodeList.getNode(0);
                while (nodePtr2 != NULL) {
                    if (nodePtr2->getNameNum() != datum)
                        nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                    nodePtr2 = nodePtr2->getNext();
                }
            }
            nodePtr1 = nodePtr1->getNext();
        }
    }






    NR_Iterations(jacMat, result, minDert, number, count, accurateValue, datum, lastnode);
    
    
    stepNum += count;

    


    if (isClose(preU, nodeValue, number, 1e-5)) {
        break;
    }

    outfile << "Time:" << endTimeFlag << endl;
    for (int i = 1; i <= number; i++) {
        outfile << "x" << i << "=" << nodeValue[i] << endl;
    }


    if (count <= 4) {
        stepSize = stepSize * 2;
        for (int i = 1; i <= number; i++) {
            preU[i] = nodeValue[i];
        }
        cout << "此时步长为：" << stepSize << "   迭代次数为：" << count << "  步长更新为：" << stepSize * 2 << endl;
    }
    else if (11 < count) {
        stepNum -= count;
        endTimeFlag -= stepSize;
        stepSize = stepSize / 8;
        for (int i = 1; i <= number; i++) {
            nodeValue[i] = preU[i];
        }

        cout << "此时步长为：" << stepSize << "   迭代次数为：" << count << "  步长更新为：" << stepSize / 8 << endl;

    }
    else {
        for (int i = 1; i <= number; i++) {
            preU[i] = nodeValue[i];
        }
        cout << "此时步长为：" << stepSize << "   迭代次数为：" << count << "  步长不变" << endl;
    }


    /*
      *
      *更新经过电容的电流
    */
    /*
    Component* comPtrs = compList.getComp(0);
    while (comPtrs != NULL) {
        if (comPtrs->getType() == Inductor) {
            Node* nodePtrs = comPtrs->getNode(0);
            Connections * conPtrs=nodePtrs->getConList();
            while (conPtrs != NULL) {
                if (conPtrs->comp->getType() == VSource) {
                    preI[comPtrs->getcompNum()] = preU[lastnode + conPtrs->comp->getcompNum()];
                }

                conPtrs = conPtrs->next;
            }



            nodePtrs = comPtrs->getNode(1);
            conPtrs = nodePtrs->getConList();
            while (conPtrs != NULL) {
                if (conPtrs->comp->getType() == VSource) {
                    preI[comPtrs->getcompNum()] = preU[lastnode + conPtrs->comp->getcompNum()];
                }

                conPtrs = conPtrs->next;
            }


        }

        comPtrs = comPtrs->getNext();
    }
   */
    
    Component* comPtrs = compList.getComp(0);
    while (comPtrs != NULL) {
        if (comPtrs->getType() == Inductor) {

            preI[comPtrs->getcompNum()] = preI[comPtrs->getcompNum()] + stepSize * (nodeValue[comPtrs->getConVal(1)] - nodeValue[comPtrs->getConVal(0)]) / comPtrs->getVal();

        }

        comPtrs = comPtrs->getNext();
    }

    

    /*******************************************************************/


    for (int i = 1; i <= number; i++) {
        nodeValue[i] = 0.0;
    }

    for (int i = 0; i < number; i++) {
        for (int j = 0; j < number; j++) {
            jacMat[i + 1][j + 1] = 0.0;
        }
        result[i + 1] = 0.0;
    }



}



outfile.close();

cout << "牛顿迭代总次数：" << stepNum<<endl;
for (int i = 1; i <= number; i++) {
    cout << nodeValue[i] << "   ";
}

}
else if (choose == 6) {




//----------------------------P_Tran-----------------------------------------------------------

//加入电容，改变电压值
cout << endl << endl << "----------------Using P-Tran Method to Solve Circuit Equations--------------------------" << endl << endl;

outfile.open("ptran.txt", ios::out);
int number = 0;
isTran = 1;      //--------------------
isChangVsoure = 1;
vsourChangIndex = 0.1;

cout << "please input data numbers:" << endl;
cin >> number;
for (int i = 1; i <= number; i++) {
    nodeValue[i] = 0.0;
}



double accurateValue = 0.0;

cout << "please input required accuracy:" << endl;
cin >> accurateValue;

    //for (double i = stepSize; i < stopTime + stepSize; i = i + stepSize) {
    while (true) {

        stepNum++;
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum) {
                nodePtr->printNodalMat(datum, lastnode, result);
            }
            nodePtr = nodePtr->getNext();
        }

        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->specialPrintMat(datum, result);
            compPtr = compPtr->getNext();
        }


        //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
        if (eqType != Modified) {
            compPtr = compList.getComp(0);
            while (compPtr != NULL) {
                compPtr->printSuperNodeMat(datum, lastnode, result);
                compPtr = compPtr->getNext();
            }
        }


        // go down the node list and give additional MNA equations
        if (eqType == Modified) {
            nodePtr = nodeList.getNode(0);
            while (nodePtr != NULL) {
                if (nodePtr->getNameNum() != datum)
                    nodePtr->printMNAMat(datum, lastnode, result);
                nodePtr = nodePtr->getNext();
            }
        }

        //求jac矩阵

        nodePtr1 = nodeList.getNode(0);
        while (nodePtr1 != NULL) {
            if (nodePtr1->getNameNum() != datum) {
                nodePtr2 = nodeList.getNode(0);
                while (nodePtr2 != NULL) {
                    if (nodePtr2->getNameNum() != datum) {
                        nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
                    }
                    nodePtr2 = nodePtr2->getNext();
                }
            }
            nodePtr1 = nodePtr1->getNext();
        }

        // go down the component list and give equations for all sources
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            nodePtr2 = nodeList.getNode(0);
            compPtr2 = compList.getComp(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum) {
                    compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
                }
                nodePtr2 = nodePtr2->getNext();
            }
            specPrintJacMNA = 0;
            compPtr = compPtr->getNext();
        }




        // print the Jacobians for the additional MNA equations
        if (eqType == Modified) {
            nodePtr1 = nodeList.getNode(0);
            while (nodePtr1 != NULL) {
                if (nodePtr1->getNameNum() != datum) {
                    nodePtr2 = nodeList.getNode(0);
                    while (nodePtr2 != NULL) {
                        if (nodePtr2->getNameNum() != datum)
                            nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                        nodePtr2 = nodePtr2->getNext();
                    }
                }
                nodePtr1 = nodePtr1->getNext();
            }
        }

        

        NR_Iterations(jacMat, result, minDert, number, count, accurateValue, datum, lastnode);
        //cout << "迭代次数：" << count << endl;

        
        if (isClose(preU, nodeValue, number, 1e-9)) {
            break;
        }

        if (isClose(preU, nodeValue, number, 1e-3)) {
            outfile << "the value of Vsour:" << vsourChangIndex << "*Value" << endl;
            for (int i = 1; i <= number; i++) {
                outfile << "x" << i << "=" << nodeValue[i] << endl;
            }

            if (vsourChangIndex < 1.0)
                vsourChangIndex += 0.1;
            else
                vsourChangIndex = 1.0;

        }

        if ( count <= 4) {
            stepSize = stepSize * 2;
            for (int i = 1; i <= number; i++) {
                preU[i] = nodeValue[i];
            }
           cout << "此时步长为：" << stepSize/2<< "   迭代次数为：" << count << "  步长更新为：" << stepSize<<endl;
        }
        else if (11 < count) {
            stepSize = stepSize / 8;
            
            
          cout << "此时步长为：" << stepSize*8 << "   迭代次数为：" << count << "  步长更新为：" << stepSize  << endl;

        }
        else {
            for (int i = 1; i <= number; i++) {
                preU[i] = nodeValue[i];
            }
            cout << "此时步长为：" << stepSize << "   迭代次数为：" << count << "  步长不变" << endl;
        }

        for (int i = 1; i <= number; i++) {
            nodeValue[i] = 0.0;
        }

        for (int i = 0; i < number; i++) {
            for (int j = 0; j < number; j++) {
                jacMat[i + 1][j + 1] = 0.0;
            }
            result[i + 1] = 0.0;
        }


    }



outfile.close();
for (int i = 1; i <= number; i++) {
    cout << nodeValue[i] << "   ";
 }

}
else if (choose == 7) {

//----------------------------P_Tran-----------------------------------------------------------

//加入电容，改变电压值
cout << endl << endl << "----------------Using P-Tran Method to Solve Circuit Equations--------------------------" << endl << endl;

outfile.open("ptran.txt", ios::out);
int number = 0;
isTran = 1;      //--------------------
isChangVsoure = 1;
vsourChangIndex = 0.0;


cout << "please input data numbers:" << endl;
cin >> number;
for (int i = 1; i <= number; i++) {
    nodeValue[i] = 0.0;
}

double accurateValue = 0.0;

cout << "please input required accuracy:" << endl;
cin >> accurateValue;

//for (double i = stepSize; i < stopTime + stepSize; i = i + stepSize) {
while (true) {

    

    endTimeFlag += stepSize;
    if (endTimeFlag > stopTime)
        // throw runtime_error("timeout");
        break;
    if (stepSize < 1e-6)
        throw runtime_error("stepSize is too small!");

    if (endTimeFlag / 400.0 < 1) {
        vsourChangIndex = endTimeFlag / 400.0;
       
    }
    else {
        vsourChangIndex = 1.0;
    }
    nodePtr = nodeList.getNode(0);
    while (nodePtr != NULL) {
        if (nodePtr->getNameNum() != datum) {
            nodePtr->printNodalMat(datum, lastnode, result);
        }
        nodePtr = nodePtr->getNext();
    }

    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        compPtr->specialPrintMat(datum, result);
        compPtr = compPtr->getNext();
    }


    //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
    if (eqType != Modified) {
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->printSuperNodeMat(datum, lastnode, result);
            compPtr = compPtr->getNext();
        }
    }


    // go down the node list and give additional MNA equations
    if (eqType == Modified) {
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum)
                nodePtr->printMNAMat(datum, lastnode, result);
            nodePtr = nodePtr->getNext();
        }
    }

    //求jac矩阵

    nodePtr1 = nodeList.getNode(0);
    while (nodePtr1 != NULL) {
        if (nodePtr1->getNameNum() != datum) {
            nodePtr2 = nodeList.getNode(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum) {
                    nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
                }
                nodePtr2 = nodePtr2->getNext();
            }
        }
        nodePtr1 = nodePtr1->getNext();
    }

    // go down the component list and give equations for all sources
    compPtr = compList.getComp(0);
    while (compPtr != NULL) {
        nodePtr2 = nodeList.getNode(0);
        compPtr2 = compList.getComp(0);
        while (nodePtr2 != NULL) {
            if (nodePtr2->getNameNum() != datum) {
                compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
            }
            nodePtr2 = nodePtr2->getNext();
        }
        specPrintJacMNA = 0;
        compPtr = compPtr->getNext();
    }




    // print the Jacobians for the additional MNA equations
    if (eqType == Modified) {
        nodePtr1 = nodeList.getNode(0);
        while (nodePtr1 != NULL) {
            if (nodePtr1->getNameNum() != datum) {
                nodePtr2 = nodeList.getNode(0);
                while (nodePtr2 != NULL) {
                    if (nodePtr2->getNameNum() != datum)
                        nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                    nodePtr2 = nodePtr2->getNext();
                }
            }
            nodePtr1 = nodePtr1->getNext();
        }
    }





    NR_Iterations(jacMat, result, minDert, number, count, accurateValue, datum, lastnode);
    //cout << "迭代次数：" << count << endl;
    stepNum += count;
    if (isClose(preU, nodeValue, number, 1e-7)) {
        break;
    }

    outfile << "the value of Vsour:" << vsourChangIndex << "*V" << endl;
    for (int i = 1; i <= number; i++) {
        outfile << "x" << i << "=" << nodeValue[i] << endl;
     }


    if (count <= 4) {
        stepSize = stepSize * 2;
        for (int i = 1; i <= number; i++) {
            preU[i] = nodeValue[i];
        }
         cout << "此时步长为：" << stepSize << "   迭代次数为：" << count << "  步长更新为：" << stepSize * 2<<endl;
    }
    else if (11 < count) {
        stepNum -= count;
        endTimeFlag -= stepSize;
        stepSize = stepSize / 8;
        for (int i = 1; i <= number; i++) {
            nodeValue[i] = preU[i];
        }

          cout << "此时步长为：" << stepSize << "   迭代次数为：" << count << "  步长更新为：" << stepSize /8  << endl;

    }
    else {
        for (int i = 1; i <= number; i++) {
            preU[i] = nodeValue[i];
        }
         cout << "此时步长为：" << stepSize << "   迭代次数为：" << count << "  步长不变" << endl;
    }

    for (int i = 1; i <= number; i++) {
        nodeValue[i] = 0.0;
    }

    for (int i = 0; i < number; i++) {
        for (int j = 0; j < number; j++) {
            jacMat[i + 1][j + 1] = 0.0;
        }
        result[i + 1] = 0.0;
    }




}


cout << "共N_R迭代：" << stepNum << "次" << endl;
outfile.close();
cout << endTimeFlag << endl;
for (int i = 1; i <= number; i++) {
    cout << nodeValue[i] << "   ";
}




}else {
cout << "input error!!!" << endl;
}


    



    return 0;
}




void NR_Iterations(double jacMat[][30], double result[], double minDert[], int number, int &count, double accurateValue, int datum
                    , int lastnode, bool Homotopy_New,bool Homotopy, double t) {



    Component* compPtr, * compPtr2;
    Node* nodePtr, * nodePtr1, * nodePtr2;
    int specPrintJacMNA = 0;
    EquaType eqType = Modified;



    /*
    cout << "*********************" << endl;
    for (int i = 1; i <= number; i++) {
        cout << nodeValue[i] << "\t";
    }
    cout << endl;
    for (int i = 1; i <= number; i++) {
        cout << result[i] << "\t";

    }
    cout << endl;
    cout << "-------------------------" << endl;
    for (int i = 1; i <= number; i++) {
        for (int j = 1; j <= number; j++) {
            cout << jacMat[i][j] << "\t";
        }
        cout << endl;
    }
    cout << "*********************" << endl;
    
    */

    double A[30][30], b[30];
    convertArray(jacMat, A, result, b, number);
    Fun(A, minDert, b, number);

    count = 1;

    for (int i = 0; i < number; i++) {
        nodeValue[i + 1] = nodeValue[i + 1] + minDert[i];
    }


    while (!isAccurate(minDert, number, accurateValue)) {

        for (int i = 0; i < number; i++) {
            for (int j = 0; j < number; j++) {
                jacMat[i + 1][j + 1] = 0.0;
            }
            result[i + 1] = 0.0;
        }
          
        if (count > 100) {
            cout << ">>>>>t=" <<t<< endl;
            break;
        }
        nodePtr = nodeList.getNode(0);
        while (nodePtr != NULL) {
            if (nodePtr->getNameNum() != datum) {
                nodePtr->printNodalMat(datum, lastnode, result);
            }
            nodePtr = nodePtr->getNext();
        }

        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            compPtr->specialPrintMat(datum, result);
            compPtr = compPtr->getNext();
        }


        //~> go down the component list and give supernode equations for all float sources (Nodal Analysis)
        if (eqType != Modified) {
            compPtr = compList.getComp(0);
            while (compPtr != NULL) {
                compPtr->printSuperNodeMat(datum, lastnode, result);
                compPtr = compPtr->getNext();
            }
        }


        // go down the node list and give additional MNA equations
        if (eqType == Modified) {
            nodePtr = nodeList.getNode(0);
            while (nodePtr != NULL) {
                if (nodePtr->getNameNum() != datum)
                    nodePtr->printMNAMat(datum, lastnode, result);
                nodePtr = nodePtr->getNext();
            }
        }





        nodePtr1 = nodeList.getNode(0);
        while (nodePtr1 != NULL) {
            if (nodePtr1->getNameNum() != datum) {
                nodePtr2 = nodeList.getNode(0);
                while (nodePtr2 != NULL) {
                    if (nodePtr2->getNameNum() != datum) {
                        nodePtr1->printJacMat(datum, nodePtr2, lastnode, eqType, jacMat);
                    }
                    nodePtr2 = nodePtr2->getNext();
                }
            }
            nodePtr1 = nodePtr1->getNext();
        }

        // go down the component list and give equations for all sources
        compPtr = compList.getComp(0);
        while (compPtr != NULL) {
            nodePtr2 = nodeList.getNode(0);
            compPtr2 = compList.getComp(0);
            while (nodePtr2 != NULL) {
                if (nodePtr2->getNameNum() != datum) {
                    compPtr->specialPrintJacMat(datum, nodePtr2, lastnode, eqType, compPtr2, &specPrintJacMNA, jacMat); // ~> specPrintJacMNA is used to verify if the jacobians w.r.t. the Modified equations was already printed to print only once.
                }
                nodePtr2 = nodePtr2->getNext();
            }
            specPrintJacMNA = 0;
            compPtr = compPtr->getNext();
        }




        // print the Jacobians for the additional MNA equations
        if (eqType == Modified) {
            nodePtr1 = nodeList.getNode(0);
            while (nodePtr1 != NULL) {
                if (nodePtr1->getNameNum() != datum) {
                    nodePtr2 = nodeList.getNode(0);
                    while (nodePtr2 != NULL) {
                        if (nodePtr2->getNameNum() != datum)
                            nodePtr1->printJacMNAMat(datum, nodePtr2, lastnode, jacMat);
                        nodePtr2 = nodePtr2->getNext();
                    }
                }
                nodePtr1 = nodePtr1->getNext();
            }
        }


        /*
        cout << "*********************" << endl;
        for (int i = 1; i <= number; i++) {
            cout << nodeValue[i] << "\t";
        }
        cout << endl;
        for (int i = 1; i <= number; i++) {
            cout << result[i] << "\t";

        }
        cout << endl;
        cout << "-------------------------" << endl;
        for (int i = 1; i <= number; i++) {
            for (int j = 1; j <= number; j++) {
                cout << jacMat[i][j] << "\t";
            }
            cout << endl;
        }
        cout << "*********************" << endl;
        */
       

        if (Homotopy_New) {
            for (int i = 1; i <= number; i++) {
                result[i] = result[i] - (1 - t) * initF[i];
            }
            convertArray(jacMat, A, result, b, number);
            Fun(A, minDert, b, number);

            for (int i = 0; i < number; i++) {
                nodeValue[i + 1] = nodeValue[i + 1] + minDert[i];
            }
 

        }
        else if (Homotopy) {
            for (int i = 1; i <= number; i++) {

                result[i] = t * result[i] + (1 - t) * G * (nodeValue[i] - a[i]);

            }



            for (int i = 1; i <= number; i++) {
                for (int j = 1; j <= number; j++) {
                    if (i == j) {

                        jacMat[i][j] = jacMat[i][j] * t + (1 - t) * G;

                    }
                    else {
                        jacMat[i][j] = jacMat[i][j] * t;
                    }

                }
            }
            convertArray(jacMat, A, result, b, number);
            Fun(A, minDert, b, number);

            for (int i = 0; i < number; i++) {
                nodeValue[i + 1] = nodeValue[i + 1] + minDert[i];
            }

        }
        else {
            

            convertArray(jacMat, A, result, b, number);
            Fun(A, minDert, b, number);

            for (int i = 0; i < number; i++) {
                nodeValue[i + 1] = nodeValue[i + 1] + minDert[i];
            }


        }

        

        count++;
        

    }
}













void convertArray(double jacMat[][30], double A[][30], double result[], double y[], int number) {
    for (int i = 0; i < number; i++) {
        for (int j = 0; j < number; j++) {
            A[i][j] = jacMat[i + 1][j + 1];
        }

        y[i] = -result[i + 1];
    }
}


void Fun(double A[][30] , double x[], double b[], int n) {
    //初始化L和U矩阵
    double L[30][30] = { 0 }, U[30][30] = { 0 }, y[30] = {0};
    for (int j = 0; j < n; j++) {
        U[0][j] = A[0][j];
        L[j][j] = 1.0;

    }
    for (int i = 1; i < n; i++) {
        if (U[0][0] != 0.0) {
            L[i][0] = A[i][0] / U[0][0];
        }
       
        
    }
    //计算 L、U 矩阵
    for (int k = 1; k < n; k++) {
        double temp = 0;
        for (int j = k; j < n; j++) {
            temp = 0;
            for (int r = 0; r < k; r++) {
                temp += L[k][r] * U[r][j];
            }
            U[k][j] = A[k][j] - temp;
        }
        for (int i = k + 1; i < n; i++) {
            temp = 0;
            for (int l = 0; l < k; l++) {
                temp += L[i][l] * U[l][k];
            }
            if (U[k][k] != 0.0) {
                L[i][k] = (A[i][k] - temp) / U[k][k];
            }
            
        }
    }
    //求矩阵y
    y[0] = b[0];
    for (int i = 1; i < n; i++) {
        double temp = 0;
        for (int l = 0; l < i; l++) {
            temp += L[i][l] * y[l];
        }
        y[i] = b[i] - temp;
    }
    //求矩阵x
    if (U[n - 1][n - 1] != 0.0) {
        x[n - 1] = y[n - 1] / U[n - 1][n - 1];
    }
   
    for (int i = n - 2; i >= 0; i--) {
        double temp = 0;
        for (int l = n - 1; l > i; l--) {
            temp += U[i][l] * x[l];
        }
        if (U[i][i] != 0.0) {
            x[i] = (y[i] - temp) / U[i][i];
        }
        
    }

}




bool isAccurate(double minDert[], int num, double acc) {
    bool re = true;
    for (int i = 0; i < num; i++) {
        if (minDert[i] > acc || -minDert[i] > acc) {
            re = false;
        }
    }
    return re;

}


bool isClose(double A[], double B[], int num, double accurate) {
    bool re = true;
    for (int i = 1; i <= num; i++) {
        if (fabs(A[i] - B[i]) > accurate) {
            re = false;
        }
    }
    return re;
}










double stripString(char* stringIn) {
    char buf[BufLength], buf2[BufLength];
    int a, b;
    strcpy(buf, stringIn);
    for (a = 0; buf[a] != '='; a++) {};
    a++;
    for (b = 0; buf[a] != '\0'; b++, a++)
        buf2[b] = buf[a];
    buf2[b] = '\0';
    return atof(buf2);    //atof()函数 将字符转换为浮点数型
};


//Print the linked list of components to check
void printComponents(Component* compPtr) {
    char compTypeName[6];
    cout << endl << "Components: " << endl << endl;
    while (compPtr != NULL) {
        strcpy(compTypeName, strComponentType(compPtr));
        cout << "->" << compTypeName << compPtr->getcompNum();
        compPtr = compPtr->getNext();
    }
    cout << endl;
    return;
}

void printNodes(Node* nodePtr, int compFlag) {

    Connections* conPtr;
    cout << endl << "Nodes: " << endl << endl;
    while (nodePtr != NULL) {
        if (compFlag == 0) { //It is printed just the names of the nodes
            cout << "-> " << nodePtr->getNameNum();
        }
        else if (compFlag == 1) { //It is printed the nodes and the connections
            cout << "-> " << nodePtr->getNameNum() << " {";
            conPtr = nodePtr->getConList();
            while (conPtr->next != NULL) {
                cout << strComponentType(conPtr->comp) << conPtr->comp->getcompNum() << ", ";
                conPtr = conPtr->next;
            }
            cout << strComponentType(conPtr->comp) << conPtr->comp->getcompNum() << '}' << endl;
        }
        else {
            cout << "Invalid value for compFlag. (0) to print just nodes, (1) to print nodes and connections!";
            exit(1);

        }

        nodePtr = nodePtr->getNext();
    }


    return;
}


char* strComponentType(Component* compPtr) {

    char* compTypeName = new char[6];
    switch (compPtr->getType()) {

    case VSource: strcpy(compTypeName, "V"); break;
    case Resistor: strcpy(compTypeName, "R"); break;
    case BJT: strcpy(compTypeName, "T"); break;
    case MOSFET: strcpy(compTypeName, "M"); break;
    case ISource: strcpy(compTypeName, "I"); break;
    case Inductor: strcpy(compTypeName, "ind"); break;
    case Diode: strcpy(compTypeName, "Diode"); break;
    case Capacitor: strcpy(compTypeName, "C"); break;
    }

    return compTypeName;
}



char* ComponentTypeName(Component* compPtr) {

    char* compTypeName = new char[6];
    switch (compPtr->getType()) {

    case VSource: strcpy(compTypeName, "VSource"); break;
    case Resistor: strcpy(compTypeName, "Resistor"); break;
    case BJT: strcpy(compTypeName, "BJT"); break;
    case MOSFET: strcpy(compTypeName, "MOSFET"); break;
    case ISource: strcpy(compTypeName, "ISource"); break;
    case Inductor: strcpy(compTypeName, "Inductor"); break;
    case Diode: strcpy(compTypeName, "Diode"); break;
    case Capacitor: strcpy(compTypeName, "Capacitor"); break;
    }

    return compTypeName;
}



int portNum(Component* comPtr, Node* nodePtr) {
    if (comPtr->getNodeNum(0) == nodePtr->getNum()) {
        return 0;
    }
    else {
        return 1;
    }
}



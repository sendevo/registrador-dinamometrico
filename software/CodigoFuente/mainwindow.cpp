#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    // Objeto puerto serie
    serialPort = 0;

    bytesDownloaded = 0;
    bytesToDownload = 0;

    loadCellMonitor = 0;

    strReceived.clear(); // String recibido
    logFilesDir = QDir::currentPath(); // Directorio para guardar archivos descargados
    // Barra de estado y ventana de mensajes de error
    statusBar = new QStatusBar;
    statusBar->showMessage("Seleccione un puerto disponible y presione \"Conectar\"");
    errorMessage = new QErrorMessage;

    portSelect = new QComboBox; // Selector de puerto serie
    portSelect->setToolTip("Elija un puerto serie disponible en la lista desplegable");

    // Explorar puertos serie conectados al equipo
    QextSerialEnumerator *serialEnumerator = new QextSerialEnumerator;
    QList<QextPortInfo> ports = serialEnumerator->getPorts();
    foreach(QextPortInfo portInfo, ports) // Para cada puerto encontrado
        if(!portInfo.portName.isEmpty()) // Si el nombre no es nulo, agregar item a la lista de puertos
            portSelect->addItem(portInfo.portName + "-" + portInfo.friendName, portInfo.portName);
    // Habilitar notificaciones de conexion/desconexion de dispositivos
    serialEnumerator->setUpNotifications();
    connect(serialEnumerator, SIGNAL(deviceDiscovered(QextPortInfo)), this, SLOT(onDeviceDiscovered()));
    connect(serialEnumerator, SIGNAL(deviceRemoved(QextPortInfo)), this, SLOT(onDeviceRemoved()));

    // Boton para conectar con el puerto seleccionado
    QPushButton *connectBtn = new QPushButton("Conectar");
    connectBtn->setToolTip("Conectar con el puerto seleccionado");
    connect(connectBtn,SIGNAL(clicked(bool)),this,SLOT(connectBtnClicked()));

    // Layout para los controles de seleccion de puerto
    QToolBar *toolBar = new QToolBar("Puerto serie");
    toolBar->addWidget(portSelect);
    toolBar->addWidget(connectBtn);

    // Tabla con la lista de archivos presentes en la memoria
    filesTable = new QTableWidget(0,3);
    QStringList hHeader;
    hHeader << "Registro" << "Nombre de archivo" << "Tamaño (bytes)";
    filesTable->setHorizontalHeaderLabels(hHeader);
    filesTable->verticalHeader()->setVisible(false);
    filesTable->setFixedWidth(480);
    filesTable->setColumnWidth(0,100);
    filesTable->setColumnWidth(1,250);
    filesTable->setEnabled(false); // Deshabilitada hasta que se descargue la lista de archivos

    // Boton para descargar el/los archivo/s seleccionado/s de la tabla
    downloadBtn = new QPushButton(QIcon("Download.png"),"");
    downloadBtn->setIconSize(QSize(40,40));
    downloadBtn->setToolTip("Descargar y guardar archivo/s seleccionado/s");
    connect(downloadBtn,SIGNAL(clicked(bool)),this,SLOT(downloadBtnClicked()));
    downloadBtn->setEnabled(false); // Deshabilitado hasta que se descargue la lista de archivos

    // Boton para borrar el/los archivo/s seleccionado/s de la tabla
    deleteBtn = new QPushButton(QIcon("Delete.png"),"");
    deleteBtn->setIconSize(QSize(40,40));
    deleteBtn->setToolTip("Borrar archivo/s seleccionado/s");
    connect(deleteBtn,SIGNAL(clicked(bool)),this,SLOT(deleteBtnClicked()));
    deleteBtn->setEnabled(false); // Deshabilitado hasta que se descargue la lista de archivos

    // Boton configurar fecha y hora del dispositivo
    setTimeBtn = new QPushButton(QIcon("Time.png"),"");
    setTimeBtn->setIconSize(QSize(40,40));
    setTimeBtn->setToolTip("Ajustar fecha y hora del dispositivo");
    connect(setTimeBtn,SIGNAL(clicked(bool)),this,SLOT(setTimeBtnClicked()));
    setTimeBtn->setEnabled(false); // Deshabilitado hasta que se descargue la lista de archivos

    cellStateBtn = new QPushButton(QIcon("Watch.png"),"");
    cellStateBtn->setIconSize(QSize(40,40));
    cellStateBtn->setToolTip("Hacer lectura de celdas de carga");
    connect(cellStateBtn,SIGNAL(clicked(bool)),this,SLOT(cellStateBtnClicked()));
    cellStateBtn->setEnabled(false);

    // Barra de progreso para la descarga de archivos
    progressBar = new QProgressBar;
    progressBar->setVisible(false);

    // Layouts
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    vBoxLayout->addWidget(downloadBtn);
    vBoxLayout->addWidget(deleteBtn);
    vBoxLayout->addWidget(setTimeBtn);
    vBoxLayout->addWidget(cellStateBtn);
    vBoxLayout->setAlignment(cellStateBtn,Qt::AlignTop); // Alinear botones hacia arriba

    QHBoxLayout *hBoxLayout = new QHBoxLayout;
    hBoxLayout->addWidget(filesTable);
    hBoxLayout->addLayout(vBoxLayout);
    hBoxLayout->setEnabled(false); // Inicialmente deshabilitada hasta que se abra el puerto

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(toolBar);
    layout->addLayout(hBoxLayout);
    layout->addWidget(statusBar);
    layout->addWidget(progressBar);

    setLayout(layout);
    setWindowTitle("Registrador LSD-INTA");
    setWindowIcon(QIcon("Inta.png"));
}

MainWindow::~MainWindow()
// Destructor de la clase
{
    if(serialPort != 0) // No se puede borrar memoria si no fue asignada
        delete serialPort;
    delete statusBar;
    delete errorMessage;
    delete filesTable;
    delete portSelect;
    delete downloadBtn;
    delete deleteBtn;
    delete setTimeBtn;
    delete cellStateBtn;
}

void MainWindow::loadPortConfiguration()
// Cargar configuracion del puerto si existe
{
    // Arreglos de configuracion del puerto
    BaudRateType baudRates[10] = {BAUD4800, BAUD9600, BAUD14400, BAUD19200, BAUD38400,
                                  BAUD56000, BAUD57600, BAUD115200, BAUD128000, BAUD256000};
    DataBitsType dataBits[4] = {DATA_5, DATA_6, DATA_7, DATA_8};
    StopBitsType stopBits[3] = {STOP_1, STOP_1_5, STOP_2};
    FlowType flowTypes[3] = {FLOW_OFF, FLOW_XONXOFF, FLOW_HARDWARE};
    ParityType parityTypes[5] = {PAR_NONE, PAR_EVEN, PAR_ODD, PAR_MARK, PAR_SPACE};

    // Leer archivo y cargar parametros
    QFile *file = new QFile("SerialPort.conf");
    file->open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream textStream(file);
    QString actualLine; // QString temporal para lectura de lineas
    int cnt = 0; // Variable temporal que cuenta las lineas leidas
    bool ok = true; // Conversion exitosa de string a int
    uint temp; // Indice para los arreglos de configuracion
    while (!textStream.atEnd()) // Recorrer el archivo
    {
        actualLine = textStream.readLine(); // Linea actual del archivo
        // Excluir comentarios y lineas vacias
        if( actualLine != "" && !actualLine.startsWith("#"))
        {
            cnt++;
            switch (cnt) {
            case 1: // Linea 1 -> Baudrate
                temp = actualLine.toInt(&ok);
                if(ok && temp < 10) serialPort->setBaudRate(baudRates[temp]);
                else serialPort->setBaudRate( BAUD9600 );
                break;
            case 2: // Linea 2 -> Databits
                temp = actualLine.toInt(&ok);
                if(ok && temp < 4) serialPort->setDataBits(dataBits[temp]);
                else serialPort->setDataBits(DATA_8);
                break;
            case 3: // Linea 3 -> Stopbits
                temp = actualLine.toInt(&ok);
                if(ok && temp < 3) serialPort->setStopBits(stopBits[temp]);
                else serialPort->setStopBits(STOP_1);
                break;
            case 4: // Linea 4 -> Flow control
                temp = actualLine.toInt(&ok);
                if(ok && temp < 3) serialPort->setFlowControl(flowTypes[temp]);
                else serialPort->setFlowControl(FLOW_OFF);
                break;
            case 5: // Linea 5 -> Parity
                temp = actualLine.toInt(&ok);
                if(ok && temp < 5) serialPort->setParity(parityTypes[temp]);
                else serialPort->setParity(PAR_NONE);
                break;
            default:
                qWarning() << "Hay lineas adicionales en el archivo de configuracion." ;
                qDebug() << "Linea :" << cnt << "Pos. :" << textStream.pos();
                break;
            }
        }
    }

    file->close();
    delete file;
    qDebug() << "Configuracion cargada.";
    qDebug() << "Baud =" << serialPort->baudRate();
    qDebug() << "Data =" << serialPort->dataBits();
    qDebug() << "Stop =" << serialPort->stopBits();
    qDebug() << "Flow =" << serialPort->flowControl();
    qDebug() << "Parity =" << serialPort->parity();
}

void MainWindow::showFileList()
// Mostrar lista de archivos en la tabla
{
    if(strListReceived.length() == 0) // Si no hay archivos
        statusBar->showMessage("No hay archivos en la memoria del dispositivo.");
    else{
        // Agregar tantas filas como archivos haya
        filesTable->setRowCount(strListReceived.length());
        QStringList fileNames; // Nombres del registro y del archivo
        for(int i = 0; i < strListReceived.length(); i++){
            // Separar nombre de registro y nombre de archivo
            fileNames = strListReceived.at(i).split(" ");
            QTableWidgetItem *newItem = new QTableWidgetItem(fileNames.at(0));
            newItem->setFlags(Qt::NoItemFlags);
            filesTable->setItem(i,0,newItem);
            filesTable->setItem(i,1,new QTableWidgetItem(fileNames[1].remove('\r')));
            newItem = new QTableWidgetItem(fileNames[2].remove('\r'));
            newItem->setFlags(Qt::NoItemFlags);
            filesTable->setItem(i,2,newItem);
        }
        strReceived.clear(); // Borrar ultimo string
        strListReceived.clear(); // Borrar lo recibido de la memoria dinamica
        filesTable->setEnabled(true);
        downloadBtn->setEnabled(true);
        deleteBtn->setEnabled(true);
        setTimeBtn->setEnabled(true);
        cellStateBtn->setEnabled(true);
        statusBar->clearMessage();
    }
}

void MainWindow::downloadFileQueue()
// Pedir a Arduino el primer archivo de la cola de archivos
{
    // Si el puerto fue creado y hay archivos para descargar, solicitar

    if(serialPort != 0){
        QString fileName = filesQueue.first();
        fileName.prepend('b').append('\n');
        serialPort->write(fileName.toStdString().c_str());
        statusBar->showMessage("Descargando archivo "+filesNameQueue.first());
        bytesToDownload = sizesQueue.first().toInt();
        qDebug() << "Por descargar" << bytesToDownload << "bytes";
        progressBar->setVisible(true);
    }
}

void MainWindow::deleteFileQueue()
// Pedir a Arduino eliminar archivos de la lista
{
    // Si el puerto fue creado y hay archivos para borrar, solicitar

    if(serialPort != 0){
        QString fileName = filesQueue.first();
        fileName.prepend('c').append('\n');
        serialPort->write(fileName.toStdString().c_str());
        statusBar->showMessage("Borrando archivo "+filesNameQueue.first());
        filesQueue.removeFirst(); // Quitar archivo borrado de la lista
        filesNameQueue.removeFirst(); // Quitar archivo borrado de la lista
        sizesQueue.removeFirst(); // Quitar archivo borrado de la lista
    }
}

void MainWindow::saveFileReceived()
// Guardar lista de strings en archivo de texto
// No debe llamarse esta funcion si no hay archivos enla lista
{
    bytesDownloaded = 0;
    // Guardar en disco el archivo que esta en memoria dinamica
    QFile *file = new QFile(logFilesDir+"/"+filesNameQueue.first());
    qDebug() << logFilesDir+"/"+filesNameQueue.first();
    file->open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream textStream(file);
    for(int i = 0; i < strListReceived.length(); i++)
        textStream << strListReceived.at(i);
    file->close(); // Cerrar archivo del disco
    delete(file); // Eliminar puntero
    strReceived.clear();
    strListReceived.clear(); // Borrar el archivo de la memoria dinamica
    filesQueue.removeFirst(); // Eliminar de la cola el archivo descargado
    filesNameQueue.removeFirst(); // Eliminar de la cola el archivo descargado
    sizesQueue.removeFirst(); // Eliminar de la cola el archivo descargado
}

void MainWindow::requestFileList()
// SLOT
{
    // Pedir a Arduino la lista de archivos
    if(serialPort != 0)
        serialPort->write("a\n");
}

void MainWindow::setRTCTime(QString amdhms)
// Ajustar hora del dispositivo: aaaammddhhmmss
{
    if(serialPort != 0){ // Si el puerto fue creado
        QString time = amdhms;
        time.prepend('e').append('\n');
        serialPort->write(time.toStdString().c_str());
    }
}

void MainWindow::downloadBtnClicked()
// SLOT: Descargar todos los archivos seleccionados
{
    logFilesDir = QFileDialog::getExistingDirectory();
    if(logFilesDir == "") return; // Si no se eligio un directorio, salir
    if(serialPort != 0){ // Si el puerto sigue conectado (serialPort debe existir!)
        for(int idx = 0; idx < filesTable->rowCount(); idx++) // Para cada archivo de la lista
            // Si esta seleccionado, poner en cola para descargar
            if(filesTable->item(idx,1)->isSelected()){
                filesQueue.append(filesTable->item(idx,0)->text());
                filesNameQueue.append(filesTable->item(idx,1)->text());
                sizesQueue.append(filesTable->item(idx,2)->text());
            }
        if(filesQueue.length() > 0){ // Si hay archivos seleccionados para descargar
            // Deshabilitar controles
            downloadBtn->setEnabled(false);
            deleteBtn->setEnabled(false);
            setTimeBtn->setEnabled(false);
            cellStateBtn->setEnabled(false);
            downloadFileQueue(); // Iniciar descarga
        }else
            errorMessage->showMessage("No ha seleccionado archivos para descargar.");
    }
}

void MainWindow::deleteBtnClicked()
// SLOT: Borrar todos los archivos seleccionados
{
    // Pedir confirmacion antes de proceder
    QMessageBox msgBox;
    msgBox.setText("Eliminar archivos de forma permanente.");
    msgBox.setInformativeText("¿Desea eliminar los archivos seleccionados?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    if(ret == QMessageBox::Ok && serialPort != 0){ // Si el puerto sigue conectado (serialPort debe existir!)
        for(int idx = 0; idx < filesTable->rowCount(); idx++) // Para cada archivo de la lista
            // Si esta seleccionado, poner en cola para eliminar
            if(filesTable->item(idx,1)->isSelected()){
                filesQueue.append(filesTable->item(idx,0)->text());
                filesNameQueue.append(filesTable->item(idx,1)->text());
                sizesQueue.append(filesTable->item(idx,2)->text());
                // eliminar fila de la tabla
                filesTable->removeRow(idx);
                idx = -1; // Al eliminar cambian los indices, hay que reiniciar contador de loop
            }
        if(filesQueue.length() > 0){
            statusBar->showMessage("Eliminando archivos...");
            // Deshabilitar controles
            downloadBtn->setEnabled(false);
            deleteBtn->setEnabled(false);
            setTimeBtn->setEnabled(false);
            cellStateBtn->setEnabled(false);
            deleteFileQueue(); // Comenzar a eliminar
        }else
            errorMessage->showMessage("No ha seleccionado archivos para eliminar.");
    }
}

void MainWindow::setTimeBtnClicked()
// SLOT: Abrir ventana de configuracion de fecha y hora del dispositivo
{
    TimeSettingWindow *timeSettingWindow = new TimeSettingWindow;
    connect(timeSettingWindow, SIGNAL(okBtnClickedSgn(QString)), this, SLOT(setRTCTime(QString)));
    timeSettingWindow->show();
}

void MainWindow::cellStateBtnClicked()
// SLOT: Abrir ventana de monitoreo de las celdas de carga
{
    if(loadCellMonitor == 0){ // Primera vez
        loadCellMonitor = new LoadCellMonitor; // Crear nueva ventana
        // La ventanta tiene un timer que dispara la solicitud de datos
        connect(loadCellMonitor, SIGNAL(sampleRequestSgn()), this, SLOT(sampleRequest()));
    }
    loadCellMonitor->show();
}

void MainWindow::sampleRequest()
// SLOT: Realizar solicitud de lectura de celdas de carga
{
    // Si el puerto existe y la ventana de monitoreo esta abierta
    if(loadCellMonitor->isActiveWindow() && serialPort != 0)
        serialPort->write("r\n"); // realizar solicitud de lectura de datos
}

void MainWindow::connectBtnClicked()
// SLOT: Conectar con el dispositivo mediante puerto serie
{
    // Crear nuevo puerto
    if(serialPort != 0) delete serialPort;
    serialPort = new QextSerialPort( portSelect->currentData().toString() );
    // Leer configuracion del archivo
    loadPortConfiguration();

    statusBar->showMessage("Conectando con puerto serie...");
    if(serialPort->open(QIODevice::ReadWrite)){
        // Pedir lista de archivos
        qDebug() << "Puerto:" << portSelect->currentData().toString();
        qDebug() << "Baud rate:" << serialPort->baudRate();
        connect(serialPort, SIGNAL(readyRead()), this, SLOT(onDataAvailable()));
        statusBar->showMessage("Solicitando lista de archivos...");
        QTimer::singleShot(3000, this, SLOT(requestFileList())); // Esperar para pedir lista
    }else
        statusBar->showMessage("El puerto serie no se pudo abrir.");
}

void MainWindow::onDeviceDiscovered()
// SLOT: Al aparecer nuevos dispositivos, actualizar lista
{
    portSelect->clear();
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
    foreach(QextPortInfo portInfo, ports) // Para cada puerto encontrado
        if(!portInfo.portName.isEmpty()) // Si el nombre no es nulo, agregar item a la lista
            portSelect->addItem(portInfo.portName + "-" + portInfo.friendName, portInfo.portName);
}

void MainWindow::onDeviceRemoved()
// SLOT: Al quitar dispositivos
{
    if(!serialPort->isOpen()){ // Si el puerto serie se cerro, eliminar objeto
        delete serialPort;
        serialPort = 0;
        statusBar->showMessage("El puerto serie fue desconectado.");
    }
}

void MainWindow::onDataAvailable()
// SLOT
{
    char c;
    int bytesAvailable = serialPort->bytesAvailable(); // Bytes disponibles para leer
    bytesDownloaded += bytesAvailable; // Contar bytes recibidos
    if(bytesToDownload != 0)
        progressBar->setValue( qRound((double) bytesDownloaded/ (double) bytesToDownload * 100) );
    for(int i = 0; i < bytesAvailable; i++){ // Para cada byte disponible
        c = serialPort->read(1).at(0); // Leer un byte
        if(c == '\n') { // Si es fin de linea
            if(strReceived.contains(EOF_CODE)){ // Si es fin de archivo
                saveFileReceived(); // Guardar en archivo de texto
                if(filesQueue.length() > 0) // Si quedan mas archivos por descargar
                    downloadFileQueue(); // Descargar siguiente
                else{ // Si no quedan mas, habilitar controles nuevamente
                    downloadBtn->setEnabled(true);
                    deleteBtn->setEnabled(true);
                    setTimeBtn->setEnabled(true);
                    cellStateBtn->setEnabled(true);
                    statusBar->clearMessage();
                    bytesToDownload = 0;
                    progressBar->setVisible(false);
                }
                return;
            }
            if(strReceived.contains(EOL_CODE)){ // Fin de lista de archivos
                showFileList(); // Cargar la lista de archivos a la tabla
                return;
            }
            if(strReceived.contains(EOD_CODE)){
                strReceived.clear();
                strListReceived.clear(); // Borrar el archivo de la memoria dinamica
                if(filesQueue.length() > 0) // Si quedan mas archivos por borrar
                    deleteFileQueue(); // Borrar siguiente
                else{ // Si no quedan mas, habilitar controles nuevamente
                    downloadBtn->setEnabled(true);
                    deleteBtn->setEnabled(true);
                    setTimeBtn->setEnabled(true);
                    cellStateBtn->setEnabled(true);
                    statusBar->clearMessage();
                }
                return;
            }
            if(strReceived.contains(EOW_CODE)){ // Fin cadena de datos
                if(loadCellMonitor != 0){ // No hace falta, pero por las dudas
                    // Mandar los 4 datos recibidos a la ventana de monitor
                    loadCellMonitor->updateBars(strListReceived.at(0).toInt(),
                                                strListReceived.at(1).toInt(),
                                                strListReceived.at(2).toInt(),
                                                strListReceived.at(3).toInt());
                }
                strReceived.clear();
                strListReceived.clear(); // Borrar el archivo de la memoria dinamica
                return;
            }
            // Si el string no contiene codigos, agregar a la lista de strings
            strListReceived.append(strReceived);
            strReceived.clear();
        } else
            strReceived.append(c);
    }
}




TimeSettingWindow::TimeSettingWindow(QWidget *parent): QWidget(parent)
{
    // Fecha y hora del sistema:
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    yearSelect = new QSpinBox;
    yearSelect->setMinimum(2000);
    yearSelect->setMaximum(3000);
    yearSelect->setValue(date.year());
    yearSelect->setToolTip("Año");

    monthSelect = new QComboBox;
    monthSelect->addItem("Ene",1);
    monthSelect->addItem("Feb",2);
    monthSelect->addItem("Mar",3);
    monthSelect->addItem("Abr",4);
    monthSelect->addItem("May",5);
    monthSelect->addItem("Jun",6);
    monthSelect->addItem("Jul",7);
    monthSelect->addItem("Ago",8);
    monthSelect->addItem("Sep",9);
    monthSelect->addItem("Oct",10);
    monthSelect->addItem("Nov",11);
    monthSelect->addItem("Dic",12);
    monthSelect->setCurrentIndex(date.month()-1);
    monthSelect->setToolTip("Mes");

    daySelect = new QSpinBox;
    daySelect->setMinimum(1);
    daySelect->setMaximum(31);
    daySelect->setValue(date.day());
    daySelect->setToolTip("Día");

    hourSelect = new QSpinBox;
    hourSelect->setMinimum(0);
    hourSelect->setMaximum(23);
    hourSelect->setValue(time.hour());
    hourSelect->setToolTip("Hora");

    minuteSelect = new QSpinBox;
    minuteSelect->setMinimum(0);
    minuteSelect->setMaximum(59);
    minuteSelect->setValue(time.minute());
    minuteSelect->setToolTip("Minuto");

    secondSelect = new QSpinBox;
    secondSelect->setMinimum(0);
    secondSelect->setMaximum(59);
    secondSelect->setValue(time.second());
    secondSelect->setToolTip("Segundo");

    QPushButton *okBtn = new QPushButton("Aceptar");
    okBtn->setToolTip("Configurar hora");
    connect(okBtn, SIGNAL(clicked(bool)), this, SLOT(okBtnClicked()));

    QPushButton *cancelBtn = new QPushButton("Cancelar");
    cancelBtn->setToolTip("Cancelar ajuste de hora");
    connect(cancelBtn, SIGNAL(clicked(bool)), this, SLOT(cancelBtnClicked()));

    /// LAYOUT ///
    QHBoxLayout *hBoxDateLayout = new QHBoxLayout;
    hBoxDateLayout->addWidget(yearSelect);
    hBoxDateLayout->addWidget(new QLabel("/"));
    hBoxDateLayout->addWidget(monthSelect);
    hBoxDateLayout->addWidget(new QLabel("/"));
    hBoxDateLayout->addWidget(daySelect);

    QHBoxLayout *hBoxTimeLayout = new QHBoxLayout;
    hBoxTimeLayout->addWidget(hourSelect);
    hBoxTimeLayout->addWidget(new QLabel(":"));
    hBoxTimeLayout->addWidget(minuteSelect);
    hBoxTimeLayout->addWidget(new QLabel(":"));
    hBoxTimeLayout->addWidget(secondSelect);

    QHBoxLayout *hBoxBtnlayout = new QHBoxLayout;
    hBoxBtnlayout->addWidget(okBtn);
    hBoxBtnlayout->addWidget(cancelBtn);

    QGroupBox *dateGrpBox = new QGroupBox("Fecha");
    dateGrpBox->setLayout(hBoxDateLayout);

    QGroupBox *timeGrpBox = new QGroupBox("Hora");
    timeGrpBox->setLayout(hBoxTimeLayout);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(dateGrpBox);
    layout->addWidget(timeGrpBox);
    layout->addLayout(hBoxBtnlayout);

    setLayout(layout);
    setWindowTitle("Ajuste de fecha y hora");
    setWindowIcon(QIcon("Time.png"));
}

TimeSettingWindow::~TimeSettingWindow()
{
    delete yearSelect;
    delete monthSelect;
    delete daySelect;
    delete hourSelect;
    delete minuteSelect;
    delete secondSelect;
}

void TimeSettingWindow::okBtnClicked()
// SLOT
{
    QString ymdhms = "";
    ymdhms = QString().setNum(yearSelect->value());
    if(monthSelect->currentData().toInt() < 10) ymdhms.append("0");
    ymdhms.append( QString().setNum(monthSelect->currentData().toInt()) );
    if(daySelect->value() < 10) ymdhms.append("0");
    ymdhms.append( QString().setNum(daySelect->value()) );
    if(hourSelect->value() < 10) ymdhms.append("0");
    ymdhms.append( QString().setNum(hourSelect->value()) );
    if(minuteSelect->value() < 10) ymdhms.append("0");
    ymdhms.append( QString().setNum(minuteSelect->value()) );
    if(secondSelect->value() < 10) ymdhms.append("0");
    ymdhms.append( QString().setNum(secondSelect->value()) );

    emit okBtnClickedSgn(ymdhms);
    this->close();
}

void TimeSettingWindow::cancelBtnClicked()
// SLOT
{
    this->close();
}



LoadCellMonitor::LoadCellMonitor(QWidget *parent): QWidget(parent)
{
    progressBarCh1 = new QProgressBar;
    progressBarCh1->setRange(0,1023);
    progressBarCh1->setFormat("Ch 1: %p% (%v)");

    progressBarCh2 = new QProgressBar;
    progressBarCh2->setRange(0,1023);
    progressBarCh2->setFormat("Ch 2: %p% (%v)");

    progressBarCh3 = new QProgressBar;
    progressBarCh3->setRange(0,1023);
    progressBarCh3->setFormat("Ch 3: %p% (%v)");

    progressBarCh4 = new QProgressBar;
    progressBarCh4->setRange(0,1023);
    progressBarCh4->setFormat("Ch 4: %p% (%v)");

    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    vBoxLayout->addWidget(progressBarCh1);
    vBoxLayout->addWidget(progressBarCh2);
    vBoxLayout->addWidget(progressBarCh3);
    vBoxLayout->addWidget(progressBarCh4);

    setLayout(vBoxLayout);
    setWindowTitle("Monitor de celdas de carga");
    setWindowIcon(QIcon("Watch.png"));

    // Timer para temporizar la lectura de las celdas de carga
    timer = new QTimer;
    timer->setInterval(1000); // Muestrear cada 1 segundo
    connect(timer,SIGNAL(timeout()),this,SLOT(sampleRequest()));
    timer->start();
}

LoadCellMonitor::~LoadCellMonitor()
{
    timer->stop();
    delete progressBarCh1;
    delete progressBarCh2;
    delete progressBarCh3;
    delete progressBarCh4;
    delete timer;
}

void LoadCellMonitor::sampleRequest()
// SLOT: Pedido de muestra -> mandar segnal a la ventana principal para que se comunique por serie
{
    emit sampleRequestSgn();
}

void LoadCellMonitor::updateBars(int bar1, int bar2, int bar3, int bar4)
{
    progressBarCh1->setValue(bar1);
    progressBarCh2->setValue(bar2);
    progressBarCh3->setValue(bar3);
    progressBarCh4->setValue(bar4);
}

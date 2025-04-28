Public Class MainWindow
    Dim comPORT As String
    Dim strReceived As String 'Linea recibida por serie
    Dim logsToDownloadList As New List(Of String) 'Cola de registros para descargar
    Dim fileNamesToDownloadList As New List(Of String) 'Cola de nombres de archivos de los registros que se van a descargar
    Dim strListReceived As New List(Of String) 'Para registrar todas las lineas recibidas por serie (Lista de archivos o contenido de archivos)
    Dim pathForDownload As String 'Ruta para descarga de archivos
    Dim monitorCellsValues(3) As Integer 'Array para guardar valores de celdas de carga durante monitoreo


    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        'Inicializar variables
        comPORT = ""
        strReceived = ""
        pathForDownload = Application.StartupPath 'Por defecto se selecciona el directorio del programa

        CheckForIllegalCrossThreadCalls = False 'Esto es para modificar el formulario desde otro Sub y que no de error

        'Enumerar puertos serie disponibles y agregarlos al selector de puertos
        For Each sp As String In My.Computer.Ports.SerialPortNames
            portComboBox.Items.Add(sp)
        Next

    End Sub

    Private Sub connectBtn_Click(sender As Object, e As EventArgs) Handles connectBtn.Click
        'Evento del boton para conectar con puerto serie
        If (portComboBox.SelectedItem <> "") Then
            StatusLabel.Text = "Configurando el puerto serie y solicitando lista de archivos..."
            SerialPort1.Close() 'Si estaba abierto, cerrar
            'Configuracion del puerto serie
            SerialPort1.PortName = portComboBox.SelectedItem
            SerialPort1.BaudRate = 9600
            SerialPort1.DataBits = 8
            SerialPort1.Parity = IO.Ports.Parity.None
            SerialPort1.StopBits = IO.Ports.StopBits.One
            SerialPort1.Handshake = IO.Ports.Handshake.None
            SerialPort1.Encoding = System.Text.Encoding.Default 'very important!
            SerialPort1.ReadTimeout = 10000
            AddHandler SerialPort1.DataReceived, AddressOf DataReceivedHandler
            'Abrir puerto serie y solicitar lista de archivos
            SerialPort1.Open()
            SerialPort1.Write("a\n")
        Else 'Si no hay puerto indicado en el selector
            MsgBox("Seleccione un puerto COM!")
        End If
    End Sub

    Private Sub downloadBtn_Click(sender As Object, e As EventArgs) Handles downloadBtn.Click
        'Evento para descargar los archivos seleccionados de la tabla
        If SerialPort1.IsOpen Then
            Dim pathDialog As New FolderBrowserDialog()
            pathDialog.RootFolder = Environment.SpecialFolder.Desktop
            pathDialog.Description = "Seleccione un directorio para descargar registros"
            If pathDialog.ShowDialog() = DialogResult.OK Then
                pathForDownload = pathDialog.SelectedPath
            End If
            'Recorrer filas y si esta seleccionada pedir el archivo correspondiente
            For rowCount As Integer = 0 To FileGridView.RowCount - 1
                If FileGridView.Item(0, rowCount).Selected = True Or
                    FileGridView.Item(1, rowCount).Selected = True Or
                    FileGridView.Item(2, rowCount).Selected = True Then
                    'Cargar filas seleccionadas a la cola de descarga
                    logsToDownloadList.Add(FileGridView.Item(0, rowCount).Value.ToString())
                    'A los nombres de archivos hay que quitarles el "\n" al final
                    Dim fileName As String = FileGridView.Item(1, rowCount).Value.ToString()
                    fileNamesToDownloadList.Add(fileName.Remove(fileName.Length - 1))
                End If
            Next
            If logsToDownloadList.Count > 0 Then 'Proceder con la descarga si hay elementos seleccionados
                'Pedir primer archivo de la cola de registros para bajar
                SerialPort1.Write("b" + logsToDownloadList.ElementAt(0)) 'El "\n" ya esta incluido
                StatusLabel.Text = "Descargando registro " + fileNamesToDownloadList.ElementAt(0) + "..."
            Else
                MsgBox("Debe seleccionar al menos un registro de la tabla!")
            End If
        Else
            MsgBox("El puerto serie no está conectado!")
        End If
    End Sub

    Private Sub deleteBtn_Click(sender As Object, e As EventArgs) Handles deleteBtn.Click
        'Evento para borrar un registro
        If SerialPort1.IsOpen Then
            'Hay que recorrer la tabla y eliminar las filas seleccionadas
            'Como el tamanio de la tabla cambia, no se puede usar un lazo for
            Dim rowList = New List(Of Integer)
            For rowCount As Integer = 0 To FileGridView.RowCount - 1
                If FileGridView.Item(0, rowCount).Selected = True Or
                    FileGridView.Item(1, rowCount).Selected = True Or
                    FileGridView.Item(2, rowCount).Selected = True Then
                    'Cargar filas seleccionadas a la cola de descarga (que ahora se usa para borrar)
                    logsToDownloadList.Add(FileGridView.Item(0, rowCount).Value.ToString())
                    'A los nombres de archivos hay que quitarles el "\n" al final
                    Dim fileName As String = FileGridView.Item(1, rowCount).Value.ToString()
                    fileNamesToDownloadList.Add(fileName.Remove(fileName.Length - 1))
                    'Agregar numero de fila actual a la lista
                    rowList.Add(rowCount)
                End If
            Next
            FileGridView.ClearSelection()
            Debug.WriteLine("rowList.Count = " + rowList.Count.ToString)
            For rowCount As Integer = 0 To rowList.Count - 1
                Debug.WriteLine("rowCount = " + rowCount.ToString)
                FileGridView.Rows.RemoveAt(rowList.ElementAt(rowCount) - rowCount)
            Next
            If logsToDownloadList.Count > 0 Then 'Proceder con la descarga si hay elementos seleccionados
                'Pedir primer archivo de la cola de registros para bajar
                SerialPort1.Write("c" + logsToDownloadList.ElementAt(0)) 'El "\n" ya esta incluido
                StatusLabel.Text = "Borrando registro " + fileNamesToDownloadList.ElementAt(0) + "..."
            Else
                MsgBox("Debe seleccionar al menos un registro de la tabla!")
            End If
        Else
            MsgBox("El puerto serie no está conectado!")
        End If
    End Sub

    Private Sub monitorBtn_Click(sender As Object, e As EventArgs) Handles monitorBtn.Click
        'Evento para mostrar monitor de celdas de carga

        'No se puede actualizar las barras de progreso desde el sub updateMonitor() asi que se actualizan desde
        'el evento que dispara el Timer1 usando un array para guardar los valores que llegan por serie
        If SerialPort1.IsOpen Then
            Monitor.Show() 'Abrir ventana con barras de progreso
            'Iniciar timer para leer celdas cada 1 segundo
            Timer1.Interval = 1000
            Timer1.Start()
            StatusLabel.Text = "Leyendo valores de celdas de carga..."
        Else
            MsgBox("El puerto serie no está conectado!")
        End If
    End Sub

    Private Sub timeBtn_Click(sender As Object, e As EventArgs) Handles timeBtn.Click
        'Evento para ajustar la fecha y hora
        DateTime.Show() 'Abrir ventana para ajustar fecha y hora del registrador
    End Sub

    Private Sub DataReceivedHandler(sender As Object, e As IO.Ports.SerialDataReceivedEventArgs)
        'Evento que se llama cada vez que el Registrador manda datos
        Dim sp As IO.Ports.SerialPort = CType(sender, IO.Ports.SerialPort)
        strReceived = sp.ReadLine
        'Debug.WriteLine("Recibido: " + strReceived)
        If strReceived.Contains("%%EOL%%") Then 'Ack para fin de listado de registros
            showFileList()
            Exit Sub
        End If
        If strReceived.Contains("##EOF##") Then 'Ack para fin de descarga del registro
            saveFile()
            Exit Sub
        End If
        If strReceived.Contains("$$EOD$$") Then 'Ack para fin de eliminacion de un registro
            deleteFile()
            Exit Sub
        End If
        If strReceived.Contains("&&EOW&&") Then 'Ack para fin de lectura de celdas de carga
            updateMonitor()
            Exit Sub
        End If
        strListReceived.Add(strReceived.Remove(strReceived.Length - 1)) 'Quitar el "\n" del final
    End Sub

    Private Sub Timer1_Tick(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Timer1.Tick
        'Evento que dispara el Timer1
        'Actualizar barras de progreso segun el arreglo con los valores recibidos por serie y volver a pedir otra lectura
        If SerialPort1.IsOpen Then
            Monitor.updateBars(monitorCellsValues(0), monitorCellsValues(1), monitorCellsValues(2), monitorCellsValues(3))
            SerialPort1.Write("r\n") 'Pedir valores de celdas de carga
        End If
    End Sub

    Private Sub showFileList()
        'Mostrar lista de archivos con propiedades en la tabla
        For rowCount As Integer = 0 To strListReceived.Count - 1
            'Crear nueva fila a partir de las 3 palabras separadas por espacio que contiene cada linea que manda el registrador
            Dim row As String() = strListReceived.ElementAt(rowCount).Split(" ")
            FileGridView.Rows.Add(row) 'Agregar la fila a la tabla (Importante: se agrega un "\n" a cada string)
        Next
        strListReceived.Clear() 'Borrar lista porque se usa luego para descargar registros
        StatusLabel.Text = "Listo."
    End Sub

    Private Sub saveFile()
        'Guardar el primer archivo de la cola de descarga
        Dim fileName As String = pathForDownload + "\" + fileNamesToDownloadList.ElementAt(0)
        StatusLabel.Text = "Archivo a guardar = " + fileName
        IO.File.WriteAllLines(fileName, strListReceived)

        'Quitar de las colas los registros descargados
        logsToDownloadList.RemoveAt(0)
        fileNamesToDownloadList.RemoveAt(0)
        strListReceived.Clear() 'Borrar registro recibido
        StatusLabel.Text = "Listo."

        'Si hay mas archivos para bajar, pedir siguiente
        If logsToDownloadList.Count > 0 Then
            SerialPort1.Write("b" + logsToDownloadList.ElementAt(0)) 'El "\n" ya esta incluido
            StatusLabel.Text = "Descargando registro " + fileNamesToDownloadList.ElementAt(0) + "..."
        End If
    End Sub

    Private Sub deleteFile()
        'Quitar de las colas los registros borrados
        logsToDownloadList.RemoveAt(0)
        fileNamesToDownloadList.RemoveAt(0)
        strListReceived.Clear() 'Borrar registro recibido
        StatusLabel.Text = "Listo."

        'Si hay mas archivos para bajar, pedir siguiente
        If logsToDownloadList.Count > 0 Then
            StatusLabel.Text = "Borrando registro " + fileNamesToDownloadList.ElementAt(0) + "..."
            SerialPort1.Write("c" + logsToDownloadList.ElementAt(0)) 'El "\n" ya esta incluido
        End If
    End Sub

    Private Sub updateMonitor()
        'Al terminar de recibir los valores por serie, actualizar el arreglo que se usa para las barras de progreso
        If strListReceived.Count >= 4 Then
            For index As Integer = 0 To 3
                monitorCellsValues(index) = Convert.ToInt32(strListReceived.ElementAt(0)) * 100 / 1023 'Rango 0-100%
            Next
        End If
        strListReceived.Clear() 'Limpiar la lista para guardar nuevas lecturas del puerto serie
    End Sub

End Class

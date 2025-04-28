<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class MainWindow
    Inherits System.Windows.Forms.Form

    'Form reemplaza a Dispose para limpiar la lista de componentes.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Requerido por el Diseñador de Windows Forms
    Private components As System.ComponentModel.IContainer

    'NOTA: el Diseñador de Windows Forms necesita el siguiente procedimiento
    'Se puede modificar usando el Diseñador de Windows Forms.  
    'No lo modifique con el editor de código.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.components = New System.ComponentModel.Container()
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(MainWindow))
        Me.portComboBox = New System.Windows.Forms.ComboBox()
        Me.SerialPort1 = New System.IO.Ports.SerialPort(Me.components)
        Me.connectBtn = New System.Windows.Forms.Button()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.downloadBtn = New System.Windows.Forms.Button()
        Me.deleteBtn = New System.Windows.Forms.Button()
        Me.timeBtn = New System.Windows.Forms.Button()
        Me.monitorBtn = New System.Windows.Forms.Button()
        Me.FileGridView = New System.Windows.Forms.DataGridView()
        Me.Registro = New System.Windows.Forms.DataGridViewTextBoxColumn()
        Me.Column2 = New System.Windows.Forms.DataGridViewTextBoxColumn()
        Me.Column3 = New System.Windows.Forms.DataGridViewTextBoxColumn()
        Me.StatusLabel = New System.Windows.Forms.Label()
        Me.Timer1 = New System.Windows.Forms.Timer(Me.components)
        CType(Me.FileGridView, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'portComboBox
        '
        Me.portComboBox.FormattingEnabled = True
        Me.portComboBox.Location = New System.Drawing.Point(84, 12)
        Me.portComboBox.Name = "portComboBox"
        Me.portComboBox.Size = New System.Drawing.Size(121, 21)
        Me.portComboBox.TabIndex = 0
        '
        'connectBtn
        '
        Me.connectBtn.Location = New System.Drawing.Point(211, 12)
        Me.connectBtn.Name = "connectBtn"
        Me.connectBtn.Size = New System.Drawing.Size(75, 21)
        Me.connectBtn.TabIndex = 1
        Me.connectBtn.Text = "Conectar"
        Me.connectBtn.UseVisualStyleBackColor = True
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(12, 15)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(66, 13)
        Me.Label1.TabIndex = 3
        Me.Label1.Text = "Puerto serie:"
        '
        'downloadBtn
        '
        Me.downloadBtn.Location = New System.Drawing.Point(531, 48)
        Me.downloadBtn.Name = "downloadBtn"
        Me.downloadBtn.Size = New System.Drawing.Size(102, 23)
        Me.downloadBtn.TabIndex = 4
        Me.downloadBtn.Text = "Descargar"
        Me.downloadBtn.UseVisualStyleBackColor = True
        '
        'deleteBtn
        '
        Me.deleteBtn.Location = New System.Drawing.Point(531, 77)
        Me.deleteBtn.Name = "deleteBtn"
        Me.deleteBtn.Size = New System.Drawing.Size(102, 23)
        Me.deleteBtn.TabIndex = 5
        Me.deleteBtn.Text = "Borrar"
        Me.deleteBtn.UseVisualStyleBackColor = True
        '
        'timeBtn
        '
        Me.timeBtn.Location = New System.Drawing.Point(531, 106)
        Me.timeBtn.Name = "timeBtn"
        Me.timeBtn.Size = New System.Drawing.Size(102, 23)
        Me.timeBtn.TabIndex = 6
        Me.timeBtn.Text = "Fecha y hora..."
        Me.timeBtn.UseVisualStyleBackColor = True
        '
        'monitorBtn
        '
        Me.monitorBtn.Location = New System.Drawing.Point(531, 135)
        Me.monitorBtn.Name = "monitorBtn"
        Me.monitorBtn.Size = New System.Drawing.Size(102, 22)
        Me.monitorBtn.TabIndex = 7
        Me.monitorBtn.Text = "Monitor de celdas"
        Me.monitorBtn.UseVisualStyleBackColor = True
        '
        'FileGridView
        '
        Me.FileGridView.AllowUserToAddRows = False
        Me.FileGridView.AllowUserToDeleteRows = False
        Me.FileGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill
        Me.FileGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize
        Me.FileGridView.Columns.AddRange(New System.Windows.Forms.DataGridViewColumn() {Me.Registro, Me.Column2, Me.Column3})
        Me.FileGridView.Location = New System.Drawing.Point(15, 48)
        Me.FileGridView.Name = "FileGridView"
        Me.FileGridView.Size = New System.Drawing.Size(510, 392)
        Me.FileGridView.TabIndex = 8
        '
        'Registro
        '
        Me.Registro.HeaderText = "Registro"
        Me.Registro.Name = "Registro"
        Me.Registro.ReadOnly = True
        '
        'Column2
        '
        Me.Column2.HeaderText = "Nombre de archivo"
        Me.Column2.Name = "Column2"
        '
        'Column3
        '
        Me.Column3.HeaderText = "Tamaño"
        Me.Column3.Name = "Column3"
        Me.Column3.ReadOnly = True
        '
        'StatusLabel
        '
        Me.StatusLabel.AutoSize = True
        Me.StatusLabel.Location = New System.Drawing.Point(15, 447)
        Me.StatusLabel.Name = "StatusLabel"
        Me.StatusLabel.Size = New System.Drawing.Size(290, 13)
        Me.StatusLabel.TabIndex = 9
        Me.StatusLabel.Text = "Seleccione un puerto serie de la lista y presione ""Conectar""."
        '
        'Timer1
        '
        '
        'MainWindow
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(645, 479)
        Me.Controls.Add(Me.StatusLabel)
        Me.Controls.Add(Me.FileGridView)
        Me.Controls.Add(Me.monitorBtn)
        Me.Controls.Add(Me.timeBtn)
        Me.Controls.Add(Me.deleteBtn)
        Me.Controls.Add(Me.downloadBtn)
        Me.Controls.Add(Me.Label1)
        Me.Controls.Add(Me.connectBtn)
        Me.Controls.Add(Me.portComboBox)
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.Name = "MainWindow"
        Me.Text = "Registrador INTA"
        CType(Me.FileGridView, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub

    Friend WithEvents portComboBox As ComboBox
    Friend WithEvents SerialPort1 As IO.Ports.SerialPort
    Friend WithEvents connectBtn As Button
    Friend WithEvents Label1 As Label
    Friend WithEvents downloadBtn As Button
    Friend WithEvents deleteBtn As Button
    Friend WithEvents timeBtn As Button
    Friend WithEvents monitorBtn As Button
    Friend WithEvents FileGridView As DataGridView
    Friend WithEvents Registro As DataGridViewTextBoxColumn
    Friend WithEvents Column2 As DataGridViewTextBoxColumn
    Friend WithEvents Column3 As DataGridViewTextBoxColumn
    Friend WithEvents StatusLabel As Label
    Friend WithEvents Timer1 As Timer
End Class

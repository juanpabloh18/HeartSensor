<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Registros de Frecuencia Cardíaca y Estado</title>
    <style>
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            background-image: url('imagenes/fondo.png'); /* Ruta de la imagen */
            background-size: cover; /* Ajusta la imagen para cubrir toda la pantalla */
            background-repeat: no-repeat; /* Evita que la imagen se repita */
            background-position: center; /* Centra la imagen en la página */
            font-family: Arial, sans-serif;
        }
        .container {
            text-align: center;
            background-color: rgba(249, 249, 249, 0.9); /* Fondo semi-transparente */
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.1);
        }
        h2 {
            color: #333;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
        }
        th, td {
            padding: 10px;
            border: 2px solid #333;
            text-align: center;
        }
        th {
            background-color: #d1d1d1;
        }
        .button {
            margin-top: 20px;
            padding: 10px 20px;
            font-size: 16px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
        .button:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
    <div class="container">
        <h2>Registros de Frecuencia Cardíaca y Estado</h2>
        
        <?php
        $usuario = "root";
        $contrasena = "";  
        $servidor = "localhost";
        $basededatos = "frecuenciaDB";

        $conexion = mysqli_connect($servidor, $usuario, $contrasena, $basededatos) or die("No se ha podido conectar al servidor de Base de datos");

        if (isset($_GET['iniciarToma'])) {
            $espIP = "192.168.1.80";  
            file_get_contents("http://$espIP/iniciar");
        }

        if (isset($_GET['frecuencia']) && isset($_GET['estado'])) {
            $frecuencia = $_GET['frecuencia'];
            $estado = $_GET['estado'];
            $consulta = "INSERT INTO frecuencia (frecuencia, estado) VALUES ('$frecuencia', '$estado')";
            mysqli_query($conexion, $consulta);
        }

        $consultaMostrar = "SELECT frecuencia AS 'frecuencia (bpm)', estado FROM frecuencia";
        $resultadoMostrar = mysqli_query($conexion, $consultaMostrar);

        echo "<table>
                <tr>
                    <th>Frecuencia (bpm)</th>
                    <th>Estado</th>
                </tr>";
        
        while ($fila = mysqli_fetch_assoc($resultadoMostrar)) {
            echo "<tr>
                    <td>" . $fila['frecuencia (bpm)'] . "</td>
                    <td>" . $fila['estado'] . "</td>
                  </tr>";
        }
        echo "</table>";

        mysqli_close($conexion);
        ?>

        <form method="GET" action="frecuencia.php">
            <button type="submit" name="iniciarToma" class="button">Iniciar Toma de Datos</button>
        </form>
    </div>
</body>
</html>

SOA Proyecto 1: Web Server Simplificado
===================================
> Raquel Elizondo Barrios  
> Carlos Martín Flores Gonzalez  
> José Daniel Salazar Vargas  
> Oscar Rodríguez Arroyo  
> Nelson Mendez Montero



## Tabla de contenidos
1. [Descripción General](#descripcion-general)
2. [Secuencial](#secuencial)
3. [_Forked_](#sforked)
4. [_Threaded_](#threaded)
5. [_Preforked_](#preforked)
6. [_Prethreaded_](#prethreaded)
7. [Cliente](#cliente)


## Descripción General
[Volver](#tabla-de-contenidos)  

- Cada una de las versiones de los servidores atiende únicamente solicitudes `HTTP GET`. 
- Cada servidor utiliza el directorio `web-resources` que viene dentro de cada proyecto como directorio "raíz" para servir los archivos.


## Secuencial
[Volver](#tabla-de-contenidos)  

Se atiende a una solicitud a la vez.

```bash
> make           # construccion
> ./secuencial   # ejecución
```  

## _Forked_
[Volver](#tabla-de-contenidos) 

Se crea un nuevo proceso para atender la solicitud.

```bash
> make           # construccion
> ./forked       # ejecución
```   

## _Threaded_
[Volver](#tabla-de-contenidos)

Se crea un nuevo _thread_ para atender la solicitud.

```bash
> make           # construccion
> ./threaded     # ejecución
```  

## _Preforked_
[Volver](#tabla-de-contenidos)

Procesos _workers_ creados a priori con _fork_ para atender las solicitudes.

```bash
> cd preforked 
> make                  #construcción
> cd target
> ./preforked 8080 5    #ejecución
```   

## _Prethreaded_
[Volver](#tabla-de-contenidos) 

Procesos _workers_ creados a priori usando _Pthreads_ para atender las solicitudes.

```bash
> make           # construccion
> ./prethreaded  # ejecución
```   

## Cliente
[Volver](#tabla-de-contenidos) 

```bash
> make           # construccion
> ./cliente      # ejecución
```  
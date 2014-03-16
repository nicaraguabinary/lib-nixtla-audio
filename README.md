#lib-nixtla-audio
======

Nixtla ("lapso de tiempo" en Nahuatl) es una librería en C para la reproducción y captura de audio que abstrae la implementación de OpenAL y OpenSL.

Esta abstracción permite crear un código que pueda ser compilado y ejecutado en iOS, Android, BB10, Windows, Linux, MacOSX y otros sistemas operativos.

#Licencia
======

LGPLv2.1, ver archivo LICENSE

#Origen
======

Publicada el 15/mar/2014.

Esta librería fue inicialmente desarrollada por Marcos Ortega a partir de experiencias en el desarrollo de aplicaciones multimedia.

#Especificaciones generales
======

- Gestión de memoria mediante un modelo de RETAIN_COUNT (RETAIN/RELEASE).
- Modelo de sources y buffers basado en OpenAL.
- Modelo de callbacks basado en OpenSL.
- Sólo dos archivos (nixtla-audio.h y nixtla-audio.c)

Adicionalmente:

- La gestión de memoria es customizable mediante las marcos NIX_MALLOC y NIX_FREE.
- El punto de invocación de los callbacks es controlado por el usuario.

#Utilidad
======

Principalmente en proyectos nativos multiplataforma, incluyendo videojuegos.

#Como integrar en un proyecto?
======

El diseño de nixtla prioriza la facilidad de integrar con otros proyectos mediante pasos simples como:

- Apuntar hacia o copiar los archivos nixtla-audio.h y nixtla-audio.c al proyecto.
- Linkear el proyecto con OpenSL (Android) u OpenAL (iOS, BB10, windows, MacOSx, Linux y otros)

#Ejemplos y demos
======

Este repositorio incluye proyectos demo para los siguientes entornos:

- XCode
- Eclipse (android)
- Eclipse (linux)
- Visual Sutido

#Contacto
======

info@nibsa.com.ni
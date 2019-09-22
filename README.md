# tp-2019-2c-Vendo-FIAT-600

![alt text](https://i.ibb.co/J2By3Hq/tp-operativos.png)

## Info que por ahí sirve de algo
* [Cómo usar los branches](docs/branches.md)
* [Repositorio de FUSE](https://github.com/libfuse/libfuse)
* [Cómo hacer para que git no te rompa las bolas con el usuario y contraseña (SSH Key)](https://help.github.com/en/enterprise/2.15/user/articles/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent)
* [Cómo cambiar el idioma del teclado en la VM](https://www.youtube.com/watch?v=aNvKk_RN2Cc)
* Cómo hacer para que git no te rompa las bolas con el usuario y contraseña (otro método del cual no me hago responsable si se rompe el repositorio)
  1. Pararse en la carpeta del repo, click derecho -> mostrar ocultos
  2. Entrar a .git/config
  3. Modificar lo que está en la parte de 'remote "origin"':
     - ```remote "origin"```
     - ```url = https://<USUARIO-GITHUB>:<CONTRASEÑA-GITHUB>@github.com/sisoputnfrba/tp-2019-2c-Vendo-FIAT-600.git```
     - ```fetch = +refs/heads/*:refs/remotes/origin/*```

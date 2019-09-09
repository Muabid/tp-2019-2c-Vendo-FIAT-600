# tp-2019-2c-Vendo-FIAT-600

![alt text](https://i.ibb.co/J2By3Hq/tp-operativos.png)

## ¿Cómo usar los branches? 


 Crear un nuevo branch
```
git branch unBranch
```

 Moverse entre los branches
```
git checkout unBranch
```


 Crear una nueva rama y saltar a ella, en un solo paso (Un atajo de los dos anteriores)
```
git checkout -b unBranch
```

 Incorporar los cambios al branch master
```
git checkout master
git merge unBranch
```

 Borrar un branch
```
git branch -d unBranch
```

"Pushear" un branch

```
git push -u origin unBranch
```

Mas info en https://git-scm.com/book/es/v1/Ramificaciones-en-Git

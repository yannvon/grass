for i in 1 3 4 5 6 7 8 9
do      
        mkdir -p baseDir
        rm -r baseDir/* 2> /dev/null
        echo
        
        if [ $i -eq 4 ];
        then    
                dd if=/dev/zero of=baseDir/server_file.txt bs=136 count=1 2> /dev/null
                dd if=/dev/zero of=client_file.txt bs=74 count=1 2> /dev/null
                dd if=/dev/zero of=client.exe bs=621 count=1 2> /dev/null
        elif [ $i -eq 6 ];
        then
                echo "server" > baseDir/server_file.txt
        elif [ $i -eq 7 ];
        then    
                mkdir baseDir/dir1
                echo "This is file 1." > baseDir/dir1/F1.txt
                echo "This is a file." > baseDir/A-F1.txt
                echo "This is file 1." > baseDir/F1.txt
                echo "This is file 2." > baseDir/F2.txt
                echo "Nothing here." > baseDir/F3.txt
                echo "Sample." > baseDir/Sample.txt
        elif [ $i -eq 8 ];
        then 
                mkdir baseDir/dir1
                mkdir baseDir/dir1/dir2
                echo "Some random text" > baseDir/F1.txt
                echo "Some other random text" > baseDir/F2.txt
                echo "This is file1." > baseDir/dir1/F1.txt
                echo "​random.guy@epfffl.ch" > baseDir/dir1/F1.txt
                echo "​random_guy@epfl.ch" > baseDir/dir1/dir2/F1.txt
                echo "​random.guy@epfl.ch" > baseDir/dir1/dir2/F2.txt
                echo "​randomguy@epfl.ch" > baseDir/dir1/F2.txt
        elif [ $i -eq 9 ];
        then
                for j in {1..9999};
                do
                        echo "This is file $j" > baseDir/File${j}.txt
                done
                
        fi
        bin/client 127.0.0.1 1337 testcases/test${i}.in testcases/test${i}_client.out
        echo
        echo "***** Test ${i} client output *****"
        echo
        cat testcases/test${i}_client.out
        echo
done        
rm -r baseDir/* 2> /dev/null        
        
        
        

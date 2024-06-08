pipeline {
    agent any
    stages {
        stage('Clone repostiroy') {
            steps {
                checkout scm
            }
        }
        stage('Arduino compile'){
            steps {
                sh "arduino-cli compile -b esp32:esp32:esp32 diplomska_arduino.ino --output-dir ./"
            }
        }
        stage('Deploy') {
            steps {
                withCredentials([usernamePassword(credentialsId: 'JENKINS', usernameVariable: 'USERNAME', passwordVariable: 'PASSWORD')]) {
                    sh 'curl -u $USERNAME:$PASSWORD -F file=@"./diplomska_arduino.ino.bin" -k https://localhost:8443/update/upload'
                }
            }
        }
    }
}
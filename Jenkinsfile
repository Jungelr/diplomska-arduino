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
                sh "arduino-cli compile ./diplomska_arduino --profile jenkinsbuild --output-dir ./diplomska_arduino/bin"
            }
        }
        stage('Deploy') {
            steps {
                withCredentials([usernamePassword(credentialsId: 'JENKINS', usernameVariable: 'USERNAME', passwordVariable: 'PASSWORD')]) {
                    sh 'curl -u $USERNAME:$PASSWORD -F file=@"./diplomska_arduino/bin/diplomska_arduino.ino.bin" -k https://localhost:8443/update/upload'
                }
            }
        }
    }
}
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
                sh "arduino-cli compile -b esp32:esp32:esp32 sketch_oct3a.ino --output-dir ./"
            }
        }
        stage('Deploy') {
            steps {
                withCredentials([usernamePassword(credentialsId: 'JENKINS', usernameVariable: 'USERNAME', passwordVariable: 'PASSWORD')]) {
                    sh 'curl -u jenkins:jenkins -F file=@"./sketch_oct3a.ino.bin" -k https://192.168.0.254:8443/update/upload'
                }
            }
        }
    }
}
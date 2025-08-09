pipeline {
    agent {
        kubernetes {
            label 'cpp-build-agent'
            yamlFile 'jenkins/pod-template.yaml'
        }
    }


    environment {
        // For secret text
        WIFI_SSID = credentials('WIFI_SSID')
        WIFI_PASSWORD = credentials('WIFI_PASSWORD')
        TEST_TCP_SERVER_IP = credentials('TEST_TCP_SERVER_IP')
    }

    stages {
        stage('Build') {
            steps {
                container('build') {
                    sh 'WIFI_SSID=$WIFI_SSID WIFI_PASSWORD=$WIFI_PASSWORD TEST_TCP_SERVER_IP=$TEST_TCP_SERVER_IP cmake -S . -B build -DPICO_BOARD=pico2_w'
                    sh 'cmake --build build'
                }
            }
        }
        stage('Test') {
            steps {
                echo "Running tests..."
                // Add test commands here, e.g., sh 'npm test' or sh './gradlew test'
            }
        }
        stage('Deploy') {
            when {
                branch 'main' // Deploy only for the 'main' branch
            }
            steps {
                echo 'Deploying application...'
                // Add deployment steps here
            }
        }
    }

    
    post {
        success {
            archiveArtifacts artifacts: 'build/main.uf2', fingerprint: true
        }
    }
}

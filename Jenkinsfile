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
        stage('Prepare Dependencies') {
            steps {
                container('build') {
                    sh """
                    if [ ! -d /cache/pico-sdk ]; then
                        echo "Downloading dependency..."
                        git clone https://github.com/raspberrypi/pico-sdk /cache/pico-sdk
                    else
                        echo "Dependency cache found, skipping download."
                        ls -la /cache
                    fi
                    """
                }
            }
        }
        stage('Build') {
            steps {
                container('build') {
                    sh 'WIFI_SSID=$WIFI_SSID WIFI_PASSWORD=$WIFI_PASSWORD TEST_TCP_SERVER_IP=$TEST_TCP_SERVER_IP cmake -S . -B build -DPICO_BOARD=pico2_w -DPICO_SDK_PATH=/cache/pico-sdk'
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

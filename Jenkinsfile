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

        DEPLOY_USER = "ricky-5"
        DEPLOY_HOST = "192.168.4.82"
        DEPLOY_PATH = "/home/ricky-5/Desktop/pico/archive/build"
    }

    stages {
        stage('Prepare Dependencies') {
            steps {
                container('build') {
                    sh """
                    if [ ! -d /cache/pico-sdk ]; then
                        echo "Downloading dependency..."
                        git clone https://github.com/raspberrypi/pico-sdk /cache/pico-sdk
                        cd /cache/pico-sdk
                        git submodule update --init
                    else
                        echo "Dependency already exists, skipping download."
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

        stage('Archive Artifact') {
            steps {
                archiveArtifacts artifacts: 'build/pico_net.uf2', fingerprint: true
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
                withCredentials([sshUserPrivateKey(credentialsId: 'jenkins-deploy-ssh', keyFileVariable: 'SSH_KEY')]) {
                    sh '''
                        scp -i $SSH_KEY -o StrictHostKeyChecking=no build/pico_net.uf2 \
                        ${DEPLOY_USER}@${DEPLOY_HOST}:${DEPLOY_PATH}/pico_net.uf2

                        ssh -i $SSH_KEY -o StrictHostKeyChecking=no ${DEPLOY_USER}@${DEPLOY_HOST} << EOF
                            picotool load -f ${DEPLOY_PATH}/pico_net.uf2
                        EOF
                    '''
                }
            }
        }

    }

    
    post {
        success {
            echo 'Pipeline completed successfully!'
        }
        failure {
            echo 'Pipeline failed. Please check the logs.'
        }
    }
}
